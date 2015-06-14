import random
import math
import sys

############################################
# Our sample city looks like this:
# x12345
# 1 | |
# 2-+-+- -->
# 3 | |
# 4-+-+- <--
# 5 | |
#
#   ^ |
#   | V
#
# At each intersection, there are traffic lights
# Some cars ride over it and all of them should
# reach their destination
############################################

north = 0
east = 1
south = 2
west = 3

district_map = {}

class City(object):
    def addRoad(self, name):
        self.exists.add(name)
        self.header.write('    std::shared_ptr<Road> road_%s;\n' % (name))
        self.sourcefile.write('    road_%s = std::make_shared<Road>(%s, "road_%s");\n' % (name, district_map[name], name))
        self.sourcefile.write('    addSubModel(road_%s);\n' % (name))

    def addResidential(self, name, path):
        #print('Found path ' + str(path))
        path = '{' + str(path)[1:-1].replace("'", '"') + '}'
        self.exists.add(name)
        self.header.write('    std::shared_ptr<Residence> residential_%s;\n' % (name))
        self.sourcefile.write('    std::vector<std::string> path_%s %s;\n' % (name, path))
        self.sourcefile.write('    residential_%s = std::make_shared<Residence>(std::make_shared<std::vector<std::string> >(path_%s), %s, "residential_%s", 100, 100, 15, 15, 15, 15);\n' % (name, name, district_map[name], name))
        self.sourcefile.write('    addSubModel(residential_%s);\n' % (name))
        self.sourcefile.write('    connectPorts(residential_%s->q_send, road_%s->q_recv_bs);\n' % (name, name))
        self.sourcefile.write('    connectPorts(residential_%s->exit, road_%s->entries);\n' %(name, name))
        self.sourcefile.write('    connectPorts(road_%s->q_sans_bs, residential_%s->q_rans);\n' % (name, name))

    def addCommercial(self, name):
        self.exists.add(name)
        self.header.write('    std::shared_ptr<Commercial> commercial_%s;\n' % (name))
        self.sourcefile.write('    commercial_%s = std::make_shared<Commercial>(%s, "commercial_%s");\n' % (name, district_map[name], name))
        self.sourcefile.write('    addSubModel(commercial_%s);\n' % (name))
        self.sourcefile.write('    connectPorts(road_%s->exits, commercial_%s->entry);\n' % (name, name))
        self.sourcefile.write('    connectPorts(commercial_%s->toCollector, collector->car_in);\n' % (name))

    def addIntersection(self, name):
        self.exists.add(name)
        self.header.write('    std::shared_ptr<Intersection> intersection_%s;\n' % (name))
        self.sourcefile.write('    intersection_%s = std::make_shared<Intersection>(%s, "intersection_%s");\n' % (name, district_map[name], name))
        self.sourcefile.write('    addSubModel(intersection_%s);\n' % (name))
        # Fetch the direction
        splitName = name.split("_")
        horizontalName = splitName [1]
        verticalName = splitName [0]
        horizontal = self.getDirection(splitName[0], True)
        vertical = self.getDirection(splitName[1], False)

        if horizontal == east:
            nameHT = str(int(horizontalName) + 1)
            nameHF = str(int(horizontalName) - 1)
        elif horizontal == west:
            nameHT = str(int(horizontalName) - 1)
            nameHF = str(int(horizontalName) + 1)
        if vertical == north:
            nameVT = str(int(verticalName) - 1)
            nameVF = str(int(verticalName) + 1)
        elif vertical == south:
            nameVT = str(int(verticalName) + 1)
            nameVF = str(int(verticalName) - 1)

        intersection = name
        roadHorizontalTo = splitName [0] + "_" + nameHT
        roadHorizontalFrom = splitName [0] + "_" + nameHF
        roadVerticalTo = nameVT + "_" + splitName [1]
        roadVerticalFrom = nameVF + "_" + splitName[1]

        self.connectIntersectionToRoad(intersection, roadHorizontalTo, horizontal)
        self.connectIntersectionToRoad(intersection, roadVerticalTo, vertical)
        self.connectRoadToIntersection(roadHorizontalFrom, intersection, horizontal)
        self.connectRoadToIntersection(roadVerticalFrom, intersection, vertical)

    def connectIntersectionToRoad(self, intersectionname, roadname, direction):
        self.sourcefile.write("    connectPorts(intersection_%s->q_send[%s], road_%s->q_recv);\n" % (intersectionname, direction, roadname))
        self.sourcefile.write("    connectPorts(road_%s->q_sans, intersection_%s->q_rans[%s]);\n" % (roadname, intersectionname, direction))
        self.sourcefile.write("    connectPorts(intersection_%s->car_out[%s], road_%s->car_in);\n" % (intersectionname, direction, roadname))

    def connectRoadToIntersection(self, roadname, intersectionname, direction):
        # Invert the direction first
        direction = {north: south, east: west, south: north, west: east}[direction]
        self.sourcefile.write("    connectPorts(road_%s->q_send, intersection_%s->q_recv[%s]);\n" % (roadname, intersectionname, direction))
        self.sourcefile.write("    connectPorts(intersection_%s->q_sans[%s], road_%s->q_rans);\n" % (intersectionname, direction, roadname))
        self.sourcefile.write("    connectPorts(road_%s->car_out, intersection_%s->car_in[%s]);\n" % (roadname, intersectionname, direction))

    def getDirection(self, name, horizontal):
        name = int(name)
        while 1:
            try:
                elem = self.directions[name]
                break
            except KeyError:
                name -= 4
                if name < 0:
                    return None
                if name % 2 != 0:
                    return None

        if horizontal:
            return elem[1] if elem[1] in [east, west] else elem[0]
        else:
            return elem[1] if elem[1] in [north, south] else elem[0]

    def setDirection(self, name, direction1, direction2):
        self.directions[int(name)] = (direction1, direction2)

    def planRoute(self, source, destination):
        sh, sv = source.split("_")
        dh, dv = destination.split("_")
        sh, sv, dh, dv = int(sh), int(sv), int(dh), int(dv)
        #print("Planning route from %s to %s" % (source, destination))

        if sv % 2 == 0:
            # We are in a vertical street
            direction = self.getDirection(sv, False)
            if direction == north:
                # The actual intersection to start from is the one north from it
                sourceIntersection = "%i_%i" % (sh-1, sv)
            elif direction == south:
                sourceIntersection = "%i_%i" % (sh+1, sv)
        else:
            # We are in a horizontal street
            direction = self.getDirection(sh, True)
            if direction == east:
                # The actual intersection to start from is the one north from it
                sourceIntersection = "%i_%i" % (sh, sv+1)
            elif direction == west:
                sourceIntersection = "%i_%i" % (sh, sv -1)

        if dv % 2 == 0:
            # We are in a vertical street
            direction = self.getDirection(dv, False)
            if direction == north:
                # The actual intersection is SOUTH from us
                destinationIntersection = "%i_%i" % (dh+1, dv)
                laststep = 'n'
            elif direction == south:
                destinationIntersection = "%i_%i" % (dh-1, dv)
                laststep = 's'
        else:
            # We are in a horizontal street
            direction = self.getDirection(dh, True)
            if direction == east:
                # The actual intersection is WEST from us
                destinationIntersection = "%i_%i" % (dh, dv -1)
                laststep = 'e'
            elif direction == west:
                destinationIntersection = "%i_%i" % (dh, dv+1)
                laststep = 'w'

        # Now plan a route from intersection 'sourceIntersection' to 'destinationIntersection'
        # furthermore add the laststep, to go to the actual location
        return self.bfSearch(sourceIntersection, destinationIntersection) + [laststep]

    def bfSearch(self, source, destination):
        #print("Search path from %s to %s" % (source, destination))
        seen = set()
        if source == destination:
            return []
        old_source, old_destination = source, destination
        steps = {north: 'n', east: 'e', south: 's', west: 'w'}
        newWorklist = [(source, [])]
        while len(newWorklist) > 0:
            worklist = newWorklist
            newWorklist = []
            for source, path in worklist:
                s_sh, s_sv = source.split("_")
                sh, sv = int(s_sh), int(s_sv)
                step1 = self.getDirection(sh, True)
                step2 = self.getDirection(sv, False)
                newsource1 = "%i_%i" % (sh, {east: sv+2, west: sv -2}[step1])
                newsource2 = "%i_%i" % ({north: sh-2, south: sh+2}[ step2], sv)
                if newsource1 in self.exists and newsource1 not in seen:
                    newWorklist.append((newsource1, list(path + [steps[step1]])))
                    seen.add(newsource1)
                    if newsource1 == destination:
                        return newWorklist [-1][1]
                if newsource2 in self.exists and newsource2 not in seen:
                    newWorklist.append((newsource2, list(path + [steps[step2]])))
                    seen.add(newsource2)
                    if newsource2 == destination:
                        return newWorklist [-1][1]

        raise KeyError("Couldn't find a route from %s to %s" % (old_source, old_destination))

    def __init__(self, districts):
        self.sourcefile = open('generated_city.cpp', 'w')
        self.sourcefile.write('#include "city.h"\n')
        self.sourcefile.write('#include <sstream>\n')

        self.header = open('generated_city.h', 'w')
        self.header.write('#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_CITY_H_\n')
        self.header.write('#define SRC_EXAMPLES_TRAFFICSYSTEM_CITY_H_\n')
        self.header.write('#include "coupledmodel.h"\n')
        self.header.write('#include "collector.h"\n')
        self.header.write('#include "residence.h"\n')
        self.header.write('#include "commercial.h"\n')
        self.header.write('#include "intersection.h"\n')
        self.header.write('#include "constants.h"\n')
        self.header.write('#include "road.h"\n')
        self.header.write('#include <vector>\n')

        self.directions = {}

        self.paths = 0
        self.setDirection("2", north, east)
        self.setDirection("4", south, west)

        x, y = int(sys.argv[1]), int(sys.argv[2])

        # NOTE for legacy reasons, x and y are reversed...
        x, y = (y if y % 2 else y + 1), (x if x % 2 else x + 1)
        nodes = int(sys.argv[4])
        district_to_node = [min(int(float(district*nodes)/districts), nodes -1) for district in range(districts)]
        print("Got map: " + str(district_to_node))
        residentialDistricts = districts/2

        self.header.write('namespace n_examples_traffic {\n')
        self.header.write('using n_model::CoupledModel;\n')
        self.header.write('using n_network::t_msgptr;\n')
        self.header.write('using n_model::t_portptr;\n')

        self.header.write("class City: public CoupledModel\n")
        self.header.write("{\n")
        self.header.write("private:\n")
        self.header.write('    std::shared_ptr<Collector> collector;\n')

        #self.sourcefile.write("int[%s] district_map = %s\n" % (len(district_to_node), district_to_node))
        self.sourcefile.write("namespace n_examples_traffic {\n")
        self.sourcefile.write("City::City(std::string name): CoupledModel(name)\n")
        self.sourcefile.write("{\n")
        self.sourcefile.write("    collector = std::make_shared<Collector>();\n")
        self.sourcefile.write("    addSubModel(collector);\n")
        self.sourcefile.write("\n")

        self.districts = [[] for _ in range(districts)]

        self.exists = set()
        # Draw horizontal streets
        for i_x in range(2, x+1, 2):
            for i_y in range(1, y+1, 2):
                district = min(int(i_x * (districts / float(x))), districts -1)
                name = "%i_%i" % (i_x, i_y)
                district_map[name] = district
                self.districts[district].append(name)
                if name not in self.exists:
                    self.addRoad(name)
                    self.exists.add(name)

        # Draw the vertical streets
        for i_y in range(2, y+1, 2):
            for i_x in range(1, x+1, 2):
                district = min(int(i_x * (districts / float(x))), districts -1)
                name = "%i_%i" % (i_x, i_y)
                district_map[name] = district
                self.districts[district].append(name)
                if name not in self.exists:
                    self.addRoad(name)
                    self.exists.add(name)

        # And the intersections
        for i_x in range(2, x+1, 2):
            for i_y in range(2, y+1, 2):
                district = min(int(i_x * (districts / float(x))), districts -1)
                name = "%i_%i" % (i_x, i_y)
                district_map[name] = district
                self.addIntersection(name)
                self.exists.add(name)

        sourceDistricts = [name for district in self.districts[: residentialDistricts] for name in district]
        destinationDistricts = [name for district in self.districts[residentialDistricts:] for name in district]
        random.shuffle(sourceDistricts)
        random.shuffle(destinationDistricts)

        paths = min(len(sourceDistricts), len(destinationDistricts))
        for source, destination in zip(sourceDistricts, destinationDistricts):
            self.addPath(source, destination)
            paths -= 1
            if paths % 100 == 0:
                print("To find: " + str(paths))

        print("Total paths: " + str(self.paths))

        self.sourcefile.write("}\n")
        self.sourcefile.write("}\n")

        self.header.write("public:\n")
        self.header.write('    City(std::string name = "City");\n')
        self.header.write('    ~City() {}\n')
        self.header.write('};\n')
        self.header.write('}\n')
        self.header.write('#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_CITY_H_ */\n')

    def addPath(self, source, destination):
        try:
            self.addResidential(source, self.planRoute(source, destination))
            self.addCommercial(destination)
            self.paths += 1
        except KeyError:
            pass

if __name__ == "__main__":
    random.seed(5)
    City(int(sys.argv[3]))