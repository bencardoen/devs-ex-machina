/*
 * firespread.cpp
 *
 *  Created on: May 4, 2015
 *      Author: lttlemoi
 */

#include "firespread.h"
#include "firecell.h"
#include "firegenerator.h"

namespace n_examples {

FireSpread::FireSpread(std::size_t sizeX, std::size_t sizeY):
	CoupledModel("FireSpread")
{
	std::vector<std::vector<t_firecellptr>> cells;
	std::vector<t_firecellptr> line;
	line.reserve(sizeY);
	cells.insert(cells.end(), sizeX, line);

	for(std::size_t x = 0; x < sizeX; ++x){
		for(std::size_t y = 0; y < sizeY; ++y) {
			t_firecellptr ptr = n_tools::createObject<FireCell>(n_model::t_point(x, y));
			addSubModel(ptr);
			cells[x].push_back(ptr);
		}
	}

	t_fgenptr generator = n_tools::createObject<FireGenerator>(RADIUS);
	addSubModel(generator);
	/*
	for level in range(RADIUS):
            # Left side
            y = level
            for x in range(-level, level + 1, 1):
                self.connectPorts(self.generator.outports[level], self.cells[CENTER[0] + x][CENTER[1] + y].inports[-1])
                self.connectPorts(self.generator.outports[level], self.cells[CENTER[0] + x][CENTER[1] - y].inports[-1])
            x = level
            for y in range(-level + 1, level, 1):
                self.connectPorts(self.generator.outports[level], self.cells[CENTER[0] + x][CENTER[1] + y].inports[-1])
                self.connectPorts(self.generator.outports[level], self.cells[CENTER[0] - x][CENTER[1] + y].inports[-1])
	 */
	std::size_t cx = sizeX/2;
	std::size_t cy = sizeY/2;
	for(std::size_t level = 0; level < RADIUS; ++level){
		std::size_t y = level;
		n_model::t_portptr& genOut = generator->getOutputs()[level];
		for(std::size_t x = 0; x < 2*level+1; ++x){
			n_model::t_portptr& cellIn = cells[cx+x-level][cy+y]->getIPorts().at("in_G");
			connectPorts(genOut, cellIn);
			cellIn = cells[cx+x-level][cy-y]->getIPorts().at("in_G");
			connectPorts(genOut, cellIn);
		}
		std::size_t x = level;
		for(y = 0; y < 2*level-1; ++y){
			n_model::t_portptr& cellIn = cells[cx+x][cy+y-level+1]->getIPorts().at("in_G");
			connectPorts(genOut, cellIn);
			cellIn = cells[cx-x][cy-+y-level+1]->getIPorts().at("in_G");
			connectPorts(genOut, cellIn);
		}
	}

	/*
	for x in range(x_max):
            for y in range(y_max):
                if x != 0:
                    self.connectPorts(self.cells[x][y].outport, self.cells[x-1][y].inports[2])
                if y != y_max - 1:
                    self.connectPorts(self.cells[x][y].outport, self.cells[x][y+1].inports[1])
                if x != x_max - 1:
                    self.connectPorts(self.cells[x][y].outport, self.cells[x+1][y].inports[3])
                if y != 0:
                    self.connectPorts(self.cells[x][y].outport, self.cells[x][y-1].inports[0])
	 */
	for(std::size_t x = 0; x < sizeX; ++x){
		for(std::size_t y = 0; y < sizeY; ++y){
			n_model::t_portptr& pOut = cells[x][y]->getOPorts().begin()->second;
			if(x){
				n_model::t_portptr& pIn = cells[x-1][y]->getIPorts()["in_S"];
				connectPorts(pOut, pIn);
			}
			if(x != sizeX-1){
				n_model::t_portptr& pIn = cells[x][y+1]->getIPorts()["in_E"];
				connectPorts(pOut, pIn);
			}
			if(y){
				n_model::t_portptr& pIn = cells[x+1][y]->getIPorts()["in_W"];
				connectPorts(pOut, pIn);
			}
			if(y != sizeY-1){
				n_model::t_portptr& pIn = cells[x][y-1]->getIPorts()["in_N"];
				connectPorts(pOut, pIn);
			}
		}
	}
}

} /* namespace n_examples */
