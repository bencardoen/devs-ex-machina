#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_CITY_H_
#define SRC_EXAMPLES_TRAFFICSYSTEM_CITY_H_
#include "coupledmodel.h"
#include "collector.h"
#include "residence.h"
#include "commercial.h"
#include "intersection.h"
#include "constants.h"
#include "road.h"
#include <vector>
namespace n_examples_traffic {
using n_model::CoupledModel;
using n_network::t_msgptr;
using n_model::t_portptr;
class City: public CoupledModel
{
private:
    std::shared_ptr<Collector> collector;
    std::shared_ptr<Road> road_2_1;
    std::shared_ptr<Road> road_2_3;
    std::shared_ptr<Road> road_2_5;
    std::shared_ptr<Road> road_2_7;
    std::shared_ptr<Road> road_2_9;
    std::shared_ptr<Road> road_2_11;
    std::shared_ptr<Road> road_4_1;
    std::shared_ptr<Road> road_4_3;
    std::shared_ptr<Road> road_4_5;
    std::shared_ptr<Road> road_4_7;
    std::shared_ptr<Road> road_4_9;
    std::shared_ptr<Road> road_4_11;
    std::shared_ptr<Road> road_6_1;
    std::shared_ptr<Road> road_6_3;
    std::shared_ptr<Road> road_6_5;
    std::shared_ptr<Road> road_6_7;
    std::shared_ptr<Road> road_6_9;
    std::shared_ptr<Road> road_6_11;
    std::shared_ptr<Road> road_8_1;
    std::shared_ptr<Road> road_8_3;
    std::shared_ptr<Road> road_8_5;
    std::shared_ptr<Road> road_8_7;
    std::shared_ptr<Road> road_8_9;
    std::shared_ptr<Road> road_8_11;
    std::shared_ptr<Road> road_10_1;
    std::shared_ptr<Road> road_10_3;
    std::shared_ptr<Road> road_10_5;
    std::shared_ptr<Road> road_10_7;
    std::shared_ptr<Road> road_10_9;
    std::shared_ptr<Road> road_10_11;
    std::shared_ptr<Road> road_12_1;
    std::shared_ptr<Road> road_12_3;
    std::shared_ptr<Road> road_12_5;
    std::shared_ptr<Road> road_12_7;
    std::shared_ptr<Road> road_12_9;
    std::shared_ptr<Road> road_12_11;
    std::shared_ptr<Road> road_14_1;
    std::shared_ptr<Road> road_14_3;
    std::shared_ptr<Road> road_14_5;
    std::shared_ptr<Road> road_14_7;
    std::shared_ptr<Road> road_14_9;
    std::shared_ptr<Road> road_14_11;
    std::shared_ptr<Road> road_16_1;
    std::shared_ptr<Road> road_16_3;
    std::shared_ptr<Road> road_16_5;
    std::shared_ptr<Road> road_16_7;
    std::shared_ptr<Road> road_16_9;
    std::shared_ptr<Road> road_16_11;
    std::shared_ptr<Road> road_18_1;
    std::shared_ptr<Road> road_18_3;
    std::shared_ptr<Road> road_18_5;
    std::shared_ptr<Road> road_18_7;
    std::shared_ptr<Road> road_18_9;
    std::shared_ptr<Road> road_18_11;
    std::shared_ptr<Road> road_20_1;
    std::shared_ptr<Road> road_20_3;
    std::shared_ptr<Road> road_20_5;
    std::shared_ptr<Road> road_20_7;
    std::shared_ptr<Road> road_20_9;
    std::shared_ptr<Road> road_20_11;
    std::shared_ptr<Road> road_22_1;
    std::shared_ptr<Road> road_22_3;
    std::shared_ptr<Road> road_22_5;
    std::shared_ptr<Road> road_22_7;
    std::shared_ptr<Road> road_22_9;
    std::shared_ptr<Road> road_22_11;
    std::shared_ptr<Road> road_24_1;
    std::shared_ptr<Road> road_24_3;
    std::shared_ptr<Road> road_24_5;
    std::shared_ptr<Road> road_24_7;
    std::shared_ptr<Road> road_24_9;
    std::shared_ptr<Road> road_24_11;
    std::shared_ptr<Road> road_26_1;
    std::shared_ptr<Road> road_26_3;
    std::shared_ptr<Road> road_26_5;
    std::shared_ptr<Road> road_26_7;
    std::shared_ptr<Road> road_26_9;
    std::shared_ptr<Road> road_26_11;
    std::shared_ptr<Road> road_28_1;
    std::shared_ptr<Road> road_28_3;
    std::shared_ptr<Road> road_28_5;
    std::shared_ptr<Road> road_28_7;
    std::shared_ptr<Road> road_28_9;
    std::shared_ptr<Road> road_28_11;
    std::shared_ptr<Road> road_30_1;
    std::shared_ptr<Road> road_30_3;
    std::shared_ptr<Road> road_30_5;
    std::shared_ptr<Road> road_30_7;
    std::shared_ptr<Road> road_30_9;
    std::shared_ptr<Road> road_30_11;
    std::shared_ptr<Road> road_32_1;
    std::shared_ptr<Road> road_32_3;
    std::shared_ptr<Road> road_32_5;
    std::shared_ptr<Road> road_32_7;
    std::shared_ptr<Road> road_32_9;
    std::shared_ptr<Road> road_32_11;
    std::shared_ptr<Road> road_34_1;
    std::shared_ptr<Road> road_34_3;
    std::shared_ptr<Road> road_34_5;
    std::shared_ptr<Road> road_34_7;
    std::shared_ptr<Road> road_34_9;
    std::shared_ptr<Road> road_34_11;
    std::shared_ptr<Road> road_36_1;
    std::shared_ptr<Road> road_36_3;
    std::shared_ptr<Road> road_36_5;
    std::shared_ptr<Road> road_36_7;
    std::shared_ptr<Road> road_36_9;
    std::shared_ptr<Road> road_36_11;
    std::shared_ptr<Road> road_38_1;
    std::shared_ptr<Road> road_38_3;
    std::shared_ptr<Road> road_38_5;
    std::shared_ptr<Road> road_38_7;
    std::shared_ptr<Road> road_38_9;
    std::shared_ptr<Road> road_38_11;
    std::shared_ptr<Road> road_40_1;
    std::shared_ptr<Road> road_40_3;
    std::shared_ptr<Road> road_40_5;
    std::shared_ptr<Road> road_40_7;
    std::shared_ptr<Road> road_40_9;
    std::shared_ptr<Road> road_40_11;
    std::shared_ptr<Road> road_42_1;
    std::shared_ptr<Road> road_42_3;
    std::shared_ptr<Road> road_42_5;
    std::shared_ptr<Road> road_42_7;
    std::shared_ptr<Road> road_42_9;
    std::shared_ptr<Road> road_42_11;
    std::shared_ptr<Road> road_44_1;
    std::shared_ptr<Road> road_44_3;
    std::shared_ptr<Road> road_44_5;
    std::shared_ptr<Road> road_44_7;
    std::shared_ptr<Road> road_44_9;
    std::shared_ptr<Road> road_44_11;
    std::shared_ptr<Road> road_46_1;
    std::shared_ptr<Road> road_46_3;
    std::shared_ptr<Road> road_46_5;
    std::shared_ptr<Road> road_46_7;
    std::shared_ptr<Road> road_46_9;
    std::shared_ptr<Road> road_46_11;
    std::shared_ptr<Road> road_48_1;
    std::shared_ptr<Road> road_48_3;
    std::shared_ptr<Road> road_48_5;
    std::shared_ptr<Road> road_48_7;
    std::shared_ptr<Road> road_48_9;
    std::shared_ptr<Road> road_48_11;
    std::shared_ptr<Road> road_50_1;
    std::shared_ptr<Road> road_50_3;
    std::shared_ptr<Road> road_50_5;
    std::shared_ptr<Road> road_50_7;
    std::shared_ptr<Road> road_50_9;
    std::shared_ptr<Road> road_50_11;
    std::shared_ptr<Road> road_1_2;
    std::shared_ptr<Road> road_3_2;
    std::shared_ptr<Road> road_5_2;
    std::shared_ptr<Road> road_7_2;
    std::shared_ptr<Road> road_9_2;
    std::shared_ptr<Road> road_11_2;
    std::shared_ptr<Road> road_13_2;
    std::shared_ptr<Road> road_15_2;
    std::shared_ptr<Road> road_17_2;
    std::shared_ptr<Road> road_19_2;
    std::shared_ptr<Road> road_21_2;
    std::shared_ptr<Road> road_23_2;
    std::shared_ptr<Road> road_25_2;
    std::shared_ptr<Road> road_27_2;
    std::shared_ptr<Road> road_29_2;
    std::shared_ptr<Road> road_31_2;
    std::shared_ptr<Road> road_33_2;
    std::shared_ptr<Road> road_35_2;
    std::shared_ptr<Road> road_37_2;
    std::shared_ptr<Road> road_39_2;
    std::shared_ptr<Road> road_41_2;
    std::shared_ptr<Road> road_43_2;
    std::shared_ptr<Road> road_45_2;
    std::shared_ptr<Road> road_47_2;
    std::shared_ptr<Road> road_49_2;
    std::shared_ptr<Road> road_51_2;
    std::shared_ptr<Road> road_1_4;
    std::shared_ptr<Road> road_3_4;
    std::shared_ptr<Road> road_5_4;
    std::shared_ptr<Road> road_7_4;
    std::shared_ptr<Road> road_9_4;
    std::shared_ptr<Road> road_11_4;
    std::shared_ptr<Road> road_13_4;
    std::shared_ptr<Road> road_15_4;
    std::shared_ptr<Road> road_17_4;
    std::shared_ptr<Road> road_19_4;
    std::shared_ptr<Road> road_21_4;
    std::shared_ptr<Road> road_23_4;
    std::shared_ptr<Road> road_25_4;
    std::shared_ptr<Road> road_27_4;
    std::shared_ptr<Road> road_29_4;
    std::shared_ptr<Road> road_31_4;
    std::shared_ptr<Road> road_33_4;
    std::shared_ptr<Road> road_35_4;
    std::shared_ptr<Road> road_37_4;
    std::shared_ptr<Road> road_39_4;
    std::shared_ptr<Road> road_41_4;
    std::shared_ptr<Road> road_43_4;
    std::shared_ptr<Road> road_45_4;
    std::shared_ptr<Road> road_47_4;
    std::shared_ptr<Road> road_49_4;
    std::shared_ptr<Road> road_51_4;
    std::shared_ptr<Road> road_1_6;
    std::shared_ptr<Road> road_3_6;
    std::shared_ptr<Road> road_5_6;
    std::shared_ptr<Road> road_7_6;
    std::shared_ptr<Road> road_9_6;
    std::shared_ptr<Road> road_11_6;
    std::shared_ptr<Road> road_13_6;
    std::shared_ptr<Road> road_15_6;
    std::shared_ptr<Road> road_17_6;
    std::shared_ptr<Road> road_19_6;
    std::shared_ptr<Road> road_21_6;
    std::shared_ptr<Road> road_23_6;
    std::shared_ptr<Road> road_25_6;
    std::shared_ptr<Road> road_27_6;
    std::shared_ptr<Road> road_29_6;
    std::shared_ptr<Road> road_31_6;
    std::shared_ptr<Road> road_33_6;
    std::shared_ptr<Road> road_35_6;
    std::shared_ptr<Road> road_37_6;
    std::shared_ptr<Road> road_39_6;
    std::shared_ptr<Road> road_41_6;
    std::shared_ptr<Road> road_43_6;
    std::shared_ptr<Road> road_45_6;
    std::shared_ptr<Road> road_47_6;
    std::shared_ptr<Road> road_49_6;
    std::shared_ptr<Road> road_51_6;
    std::shared_ptr<Road> road_1_8;
    std::shared_ptr<Road> road_3_8;
    std::shared_ptr<Road> road_5_8;
    std::shared_ptr<Road> road_7_8;
    std::shared_ptr<Road> road_9_8;
    std::shared_ptr<Road> road_11_8;
    std::shared_ptr<Road> road_13_8;
    std::shared_ptr<Road> road_15_8;
    std::shared_ptr<Road> road_17_8;
    std::shared_ptr<Road> road_19_8;
    std::shared_ptr<Road> road_21_8;
    std::shared_ptr<Road> road_23_8;
    std::shared_ptr<Road> road_25_8;
    std::shared_ptr<Road> road_27_8;
    std::shared_ptr<Road> road_29_8;
    std::shared_ptr<Road> road_31_8;
    std::shared_ptr<Road> road_33_8;
    std::shared_ptr<Road> road_35_8;
    std::shared_ptr<Road> road_37_8;
    std::shared_ptr<Road> road_39_8;
    std::shared_ptr<Road> road_41_8;
    std::shared_ptr<Road> road_43_8;
    std::shared_ptr<Road> road_45_8;
    std::shared_ptr<Road> road_47_8;
    std::shared_ptr<Road> road_49_8;
    std::shared_ptr<Road> road_51_8;
    std::shared_ptr<Road> road_1_10;
    std::shared_ptr<Road> road_3_10;
    std::shared_ptr<Road> road_5_10;
    std::shared_ptr<Road> road_7_10;
    std::shared_ptr<Road> road_9_10;
    std::shared_ptr<Road> road_11_10;
    std::shared_ptr<Road> road_13_10;
    std::shared_ptr<Road> road_15_10;
    std::shared_ptr<Road> road_17_10;
    std::shared_ptr<Road> road_19_10;
    std::shared_ptr<Road> road_21_10;
    std::shared_ptr<Road> road_23_10;
    std::shared_ptr<Road> road_25_10;
    std::shared_ptr<Road> road_27_10;
    std::shared_ptr<Road> road_29_10;
    std::shared_ptr<Road> road_31_10;
    std::shared_ptr<Road> road_33_10;
    std::shared_ptr<Road> road_35_10;
    std::shared_ptr<Road> road_37_10;
    std::shared_ptr<Road> road_39_10;
    std::shared_ptr<Road> road_41_10;
    std::shared_ptr<Road> road_43_10;
    std::shared_ptr<Road> road_45_10;
    std::shared_ptr<Road> road_47_10;
    std::shared_ptr<Road> road_49_10;
    std::shared_ptr<Road> road_51_10;
    std::shared_ptr<Intersection> intersection_2_2;
    std::shared_ptr<Intersection> intersection_2_4;
    std::shared_ptr<Intersection> intersection_2_6;
    std::shared_ptr<Intersection> intersection_2_8;
    std::shared_ptr<Intersection> intersection_2_10;
    std::shared_ptr<Intersection> intersection_4_2;
    std::shared_ptr<Intersection> intersection_4_4;
    std::shared_ptr<Intersection> intersection_4_6;
    std::shared_ptr<Intersection> intersection_4_8;
    std::shared_ptr<Intersection> intersection_4_10;
    std::shared_ptr<Intersection> intersection_6_2;
    std::shared_ptr<Intersection> intersection_6_4;
    std::shared_ptr<Intersection> intersection_6_6;
    std::shared_ptr<Intersection> intersection_6_8;
    std::shared_ptr<Intersection> intersection_6_10;
    std::shared_ptr<Intersection> intersection_8_2;
    std::shared_ptr<Intersection> intersection_8_4;
    std::shared_ptr<Intersection> intersection_8_6;
    std::shared_ptr<Intersection> intersection_8_8;
    std::shared_ptr<Intersection> intersection_8_10;
    std::shared_ptr<Intersection> intersection_10_2;
    std::shared_ptr<Intersection> intersection_10_4;
    std::shared_ptr<Intersection> intersection_10_6;
    std::shared_ptr<Intersection> intersection_10_8;
    std::shared_ptr<Intersection> intersection_10_10;
    std::shared_ptr<Intersection> intersection_12_2;
    std::shared_ptr<Intersection> intersection_12_4;
    std::shared_ptr<Intersection> intersection_12_6;
    std::shared_ptr<Intersection> intersection_12_8;
    std::shared_ptr<Intersection> intersection_12_10;
    std::shared_ptr<Intersection> intersection_14_2;
    std::shared_ptr<Intersection> intersection_14_4;
    std::shared_ptr<Intersection> intersection_14_6;
    std::shared_ptr<Intersection> intersection_14_8;
    std::shared_ptr<Intersection> intersection_14_10;
    std::shared_ptr<Intersection> intersection_16_2;
    std::shared_ptr<Intersection> intersection_16_4;
    std::shared_ptr<Intersection> intersection_16_6;
    std::shared_ptr<Intersection> intersection_16_8;
    std::shared_ptr<Intersection> intersection_16_10;
    std::shared_ptr<Intersection> intersection_18_2;
    std::shared_ptr<Intersection> intersection_18_4;
    std::shared_ptr<Intersection> intersection_18_6;
    std::shared_ptr<Intersection> intersection_18_8;
    std::shared_ptr<Intersection> intersection_18_10;
    std::shared_ptr<Intersection> intersection_20_2;
    std::shared_ptr<Intersection> intersection_20_4;
    std::shared_ptr<Intersection> intersection_20_6;
    std::shared_ptr<Intersection> intersection_20_8;
    std::shared_ptr<Intersection> intersection_20_10;
    std::shared_ptr<Intersection> intersection_22_2;
    std::shared_ptr<Intersection> intersection_22_4;
    std::shared_ptr<Intersection> intersection_22_6;
    std::shared_ptr<Intersection> intersection_22_8;
    std::shared_ptr<Intersection> intersection_22_10;
    std::shared_ptr<Intersection> intersection_24_2;
    std::shared_ptr<Intersection> intersection_24_4;
    std::shared_ptr<Intersection> intersection_24_6;
    std::shared_ptr<Intersection> intersection_24_8;
    std::shared_ptr<Intersection> intersection_24_10;
    std::shared_ptr<Intersection> intersection_26_2;
    std::shared_ptr<Intersection> intersection_26_4;
    std::shared_ptr<Intersection> intersection_26_6;
    std::shared_ptr<Intersection> intersection_26_8;
    std::shared_ptr<Intersection> intersection_26_10;
    std::shared_ptr<Intersection> intersection_28_2;
    std::shared_ptr<Intersection> intersection_28_4;
    std::shared_ptr<Intersection> intersection_28_6;
    std::shared_ptr<Intersection> intersection_28_8;
    std::shared_ptr<Intersection> intersection_28_10;
    std::shared_ptr<Intersection> intersection_30_2;
    std::shared_ptr<Intersection> intersection_30_4;
    std::shared_ptr<Intersection> intersection_30_6;
    std::shared_ptr<Intersection> intersection_30_8;
    std::shared_ptr<Intersection> intersection_30_10;
    std::shared_ptr<Intersection> intersection_32_2;
    std::shared_ptr<Intersection> intersection_32_4;
    std::shared_ptr<Intersection> intersection_32_6;
    std::shared_ptr<Intersection> intersection_32_8;
    std::shared_ptr<Intersection> intersection_32_10;
    std::shared_ptr<Intersection> intersection_34_2;
    std::shared_ptr<Intersection> intersection_34_4;
    std::shared_ptr<Intersection> intersection_34_6;
    std::shared_ptr<Intersection> intersection_34_8;
    std::shared_ptr<Intersection> intersection_34_10;
    std::shared_ptr<Intersection> intersection_36_2;
    std::shared_ptr<Intersection> intersection_36_4;
    std::shared_ptr<Intersection> intersection_36_6;
    std::shared_ptr<Intersection> intersection_36_8;
    std::shared_ptr<Intersection> intersection_36_10;
    std::shared_ptr<Intersection> intersection_38_2;
    std::shared_ptr<Intersection> intersection_38_4;
    std::shared_ptr<Intersection> intersection_38_6;
    std::shared_ptr<Intersection> intersection_38_8;
    std::shared_ptr<Intersection> intersection_38_10;
    std::shared_ptr<Intersection> intersection_40_2;
    std::shared_ptr<Intersection> intersection_40_4;
    std::shared_ptr<Intersection> intersection_40_6;
    std::shared_ptr<Intersection> intersection_40_8;
    std::shared_ptr<Intersection> intersection_40_10;
    std::shared_ptr<Intersection> intersection_42_2;
    std::shared_ptr<Intersection> intersection_42_4;
    std::shared_ptr<Intersection> intersection_42_6;
    std::shared_ptr<Intersection> intersection_42_8;
    std::shared_ptr<Intersection> intersection_42_10;
    std::shared_ptr<Intersection> intersection_44_2;
    std::shared_ptr<Intersection> intersection_44_4;
    std::shared_ptr<Intersection> intersection_44_6;
    std::shared_ptr<Intersection> intersection_44_8;
    std::shared_ptr<Intersection> intersection_44_10;
    std::shared_ptr<Intersection> intersection_46_2;
    std::shared_ptr<Intersection> intersection_46_4;
    std::shared_ptr<Intersection> intersection_46_6;
    std::shared_ptr<Intersection> intersection_46_8;
    std::shared_ptr<Intersection> intersection_46_10;
    std::shared_ptr<Intersection> intersection_48_2;
    std::shared_ptr<Intersection> intersection_48_4;
    std::shared_ptr<Intersection> intersection_48_6;
    std::shared_ptr<Intersection> intersection_48_8;
    std::shared_ptr<Intersection> intersection_48_10;
    std::shared_ptr<Intersection> intersection_50_2;
    std::shared_ptr<Intersection> intersection_50_4;
    std::shared_ptr<Intersection> intersection_50_6;
    std::shared_ptr<Intersection> intersection_50_8;
    std::shared_ptr<Intersection> intersection_50_10;
    std::shared_ptr<Residence> residential_12_7;
    std::shared_ptr<Commercial> commercial_42_9;
    std::shared_ptr<Residence> residential_22_7;
    std::shared_ptr<Commercial> commercial_31_6;
    std::shared_ptr<Residence> residential_10_9;
    std::shared_ptr<Commercial> commercial_32_1;
    std::shared_ptr<Residence> residential_10_3;
    std::shared_ptr<Commercial> commercial_33_8;
    std::shared_ptr<Residence> residential_22_5;
    std::shared_ptr<Commercial> commercial_37_4;
    std::shared_ptr<Residence> residential_24_3;
    std::shared_ptr<Commercial> commercial_29_10;
    std::shared_ptr<Residence> residential_25_6;
    std::shared_ptr<Commercial> commercial_30_9;
    std::shared_ptr<Residence> residential_25_4;
    std::shared_ptr<Commercial> commercial_48_7;
    std::shared_ptr<Residence> residential_3_2;
    std::shared_ptr<Commercial> commercial_40_5;
    std::shared_ptr<Residence> residential_17_6;
    std::shared_ptr<Commercial> commercial_39_6;
    std::shared_ptr<Residence> residential_10_7;
    std::shared_ptr<Commercial> commercial_36_5;
    std::shared_ptr<Residence> residential_12_3;
    std::shared_ptr<Commercial> commercial_27_4;
    std::shared_ptr<Residence> residential_22_1;
    std::shared_ptr<Commercial> commercial_34_11;
    std::shared_ptr<Residence> residential_22_3;
    std::shared_ptr<Commercial> commercial_43_2;
    std::shared_ptr<Residence> residential_18_1;
    std::shared_ptr<Commercial> commercial_33_10;
    std::shared_ptr<Residence> residential_23_4;
    std::shared_ptr<Commercial> commercial_41_4;
    std::shared_ptr<Residence> residential_5_6;
    std::shared_ptr<Commercial> commercial_27_6;
    std::shared_ptr<Residence> residential_19_8;
    std::shared_ptr<Commercial> commercial_48_9;
    std::shared_ptr<Residence> residential_8_9;
    std::shared_ptr<Commercial> commercial_36_3;
    std::shared_ptr<Residence> residential_25_8;
    std::shared_ptr<Commercial> commercial_33_6;
    std::shared_ptr<Residence> residential_18_7;
    std::shared_ptr<Commercial> commercial_28_1;
    std::shared_ptr<Residence> residential_24_5;
    std::shared_ptr<Commercial> commercial_27_10;
    std::shared_ptr<Residence> residential_16_11;
    std::shared_ptr<Commercial> commercial_33_4;
    std::shared_ptr<Residence> residential_7_10;
    std::shared_ptr<Commercial> commercial_32_3;
    std::shared_ptr<Residence> residential_25_2;
    std::shared_ptr<Commercial> commercial_35_4;
    std::shared_ptr<Residence> residential_18_9;
    std::shared_ptr<Commercial> commercial_51_4;
    std::shared_ptr<Residence> residential_14_3;
    std::shared_ptr<Commercial> commercial_29_4;
    std::shared_ptr<Residence> residential_7_6;
    std::shared_ptr<Commercial> commercial_39_2;
    std::shared_ptr<Residence> residential_2_7;
    std::shared_ptr<Commercial> commercial_41_2;
    std::shared_ptr<Residence> residential_9_8;
    std::shared_ptr<Commercial> commercial_48_5;
    std::shared_ptr<Residence> residential_4_7;
    std::shared_ptr<Commercial> commercial_47_8;
    std::shared_ptr<Residence> residential_7_8;
    std::shared_ptr<Commercial> commercial_50_7;
    std::shared_ptr<Residence> residential_14_1;
    std::shared_ptr<Commercial> commercial_37_6;
    std::shared_ptr<Residence> residential_15_10;
    std::shared_ptr<Commercial> commercial_45_10;
    std::shared_ptr<Residence> residential_14_5;
    std::shared_ptr<Commercial> commercial_26_11;
    std::shared_ptr<Residence> residential_2_5;
    std::shared_ptr<Commercial> commercial_39_10;
    std::shared_ptr<Residence> residential_12_11;
    std::shared_ptr<Commercial> commercial_48_3;
    std::shared_ptr<Residence> residential_21_4;
    std::shared_ptr<Commercial> commercial_29_6;
    std::shared_ptr<Residence> residential_9_2;
    std::shared_ptr<Commercial> commercial_43_4;
    std::shared_ptr<Residence> residential_11_4;
    std::shared_ptr<Commercial> commercial_36_7;
    std::shared_ptr<Residence> residential_6_9;
    std::shared_ptr<Commercial> commercial_30_11;
    std::shared_ptr<Residence> residential_4_3;
    std::shared_ptr<Commercial> commercial_26_9;
    std::shared_ptr<Residence> residential_15_6;
    std::shared_ptr<Commercial> commercial_31_10;
    std::shared_ptr<Residence> residential_10_5;
    std::shared_ptr<Commercial> commercial_47_6;
    std::shared_ptr<Residence> residential_9_10;
    std::shared_ptr<Commercial> commercial_49_4;
    std::shared_ptr<Residence> residential_9_4;
    std::shared_ptr<Commercial> commercial_27_2;
    std::shared_ptr<Residence> residential_8_11;
    std::shared_ptr<Commercial> commercial_30_3;
    std::shared_ptr<Residence> residential_2_3;
    std::shared_ptr<Commercial> commercial_46_5;
    std::shared_ptr<Residence> residential_9_6;
    std::shared_ptr<Commercial> commercial_30_5;
    std::shared_ptr<Residence> residential_12_5;
    std::shared_ptr<Commercial> commercial_36_1;
    std::shared_ptr<Residence> residential_4_11;
    std::shared_ptr<Commercial> commercial_28_7;
    std::shared_ptr<Residence> residential_13_4;
    std::shared_ptr<Commercial> commercial_41_10;
    std::shared_ptr<Residence> residential_24_7;
    std::shared_ptr<Commercial> commercial_33_2;
    std::shared_ptr<Residence> residential_10_1;
    std::shared_ptr<Commercial> commercial_30_7;
    std::shared_ptr<Residence> residential_20_5;
    std::shared_ptr<Commercial> commercial_31_8;
    std::shared_ptr<Residence> residential_3_4;
    std::shared_ptr<Commercial> commercial_44_5;
    std::shared_ptr<Residence> residential_8_7;
    std::shared_ptr<Commercial> commercial_27_8;
    std::shared_ptr<Residence> residential_23_8;
    std::shared_ptr<Commercial> commercial_49_6;
    std::shared_ptr<Residence> residential_15_4;
    std::shared_ptr<Commercial> commercial_45_8;
    std::shared_ptr<Residence> residential_12_9;
    std::shared_ptr<Commercial> commercial_26_7;
    std::shared_ptr<Residence> residential_15_8;
    std::shared_ptr<Commercial> commercial_50_5;
    std::shared_ptr<Residence> residential_15_2;
    std::shared_ptr<Commercial> commercial_29_2;
    std::shared_ptr<Residence> residential_8_3;
    std::shared_ptr<Commercial> commercial_28_5;
    std::shared_ptr<Residence> residential_25_10;
    std::shared_ptr<Commercial> commercial_26_5;
    std::shared_ptr<Residence> residential_19_6;
    std::shared_ptr<Commercial> commercial_49_8;
    std::shared_ptr<Residence> residential_6_5;
    std::shared_ptr<Commercial> commercial_35_10;
    std::shared_ptr<Residence> residential_11_10;
    std::shared_ptr<Commercial> commercial_45_4;
    std::shared_ptr<Residence> residential_21_2;
    std::shared_ptr<Commercial> commercial_36_9;
    std::shared_ptr<Residence> residential_2_1;
    std::shared_ptr<Commercial> commercial_28_9;
    std::shared_ptr<Residence> residential_3_6;
    std::shared_ptr<Commercial> commercial_46_7;
    std::shared_ptr<Residence> residential_21_8;
    std::shared_ptr<Commercial> commercial_38_3;
    std::shared_ptr<Residence> residential_7_4;
    std::shared_ptr<Commercial> commercial_37_10;
    std::shared_ptr<Residence> residential_6_3;
    std::shared_ptr<Commercial> commercial_34_3;
    std::shared_ptr<Residence> residential_19_10;
    std::shared_ptr<Commercial> commercial_40_9;
    std::shared_ptr<Residence> residential_18_3;
    std::shared_ptr<Commercial> commercial_44_7;
    std::shared_ptr<Residence> residential_4_9;
    std::shared_ptr<Commercial> commercial_35_6;
    std::shared_ptr<Residence> residential_21_6;
    std::shared_ptr<Commercial> commercial_43_8;
    std::shared_ptr<Residence> residential_11_2;
    std::shared_ptr<Commercial> commercial_44_3;
    std::shared_ptr<Residence> residential_19_2;
    std::shared_ptr<Commercial> commercial_38_11;
    std::shared_ptr<Residence> residential_23_6;
    std::shared_ptr<Commercial> commercial_49_10;
    std::shared_ptr<Residence> residential_17_10;
    std::shared_ptr<Commercial> commercial_50_9;
    std::shared_ptr<Residence> residential_5_4;
    std::shared_ptr<Commercial> commercial_46_3;
    std::shared_ptr<Residence> residential_5_2;
    std::shared_ptr<Commercial> commercial_40_7;
    std::shared_ptr<Residence> residential_23_10;
    std::shared_ptr<Commercial> commercial_51_8;
    std::shared_ptr<Residence> residential_13_8;
    std::shared_ptr<Commercial> commercial_39_4;
    std::shared_ptr<Residence> residential_17_8;
    std::shared_ptr<Commercial> commercial_35_8;
    std::shared_ptr<Residence> residential_4_5;
    std::shared_ptr<Commercial> commercial_40_1;
    std::shared_ptr<Residence> residential_17_4;
    std::shared_ptr<Commercial> commercial_31_4;
    std::shared_ptr<Residence> residential_20_9;
    std::shared_ptr<Commercial> commercial_37_8;
    std::shared_ptr<Residence> residential_7_2;
    std::shared_ptr<Commercial> commercial_26_3;
    std::shared_ptr<Residence> residential_5_10;
    std::shared_ptr<Commercial> commercial_38_7;
    std::shared_ptr<Residence> residential_1_4;
    std::shared_ptr<Commercial> commercial_40_3;
    std::shared_ptr<Residence> residential_13_10;
    std::shared_ptr<Commercial> commercial_46_11;
    std::shared_ptr<Residence> residential_13_2;
    std::shared_ptr<Commercial> commercial_32_7;
    std::shared_ptr<Residence> residential_6_7;
    std::shared_ptr<Commercial> commercial_41_6;
    std::shared_ptr<Residence> residential_11_8;
    std::shared_ptr<Commercial> commercial_43_6;
    std::shared_ptr<Residence> residential_3_8;
    std::shared_ptr<Commercial> commercial_37_2;
    std::shared_ptr<Residence> residential_21_10;
    std::shared_ptr<Commercial> commercial_32_5;
    std::shared_ptr<Residence> residential_16_3;
    std::shared_ptr<Commercial> commercial_42_5;
    std::shared_ptr<Residence> residential_24_11;
    std::shared_ptr<Commercial> commercial_34_5;
    std::shared_ptr<Residence> residential_1_8;
    std::shared_ptr<Commercial> commercial_41_8;
    std::shared_ptr<Residence> residential_23_2;
    std::shared_ptr<Commercial> commercial_42_7;
    std::shared_ptr<Residence> residential_20_7;
    std::shared_ptr<Commercial> commercial_34_9;
    std::shared_ptr<Residence> residential_19_4;
    std::shared_ptr<Commercial> commercial_47_4;
    std::shared_ptr<Residence> residential_16_7;
    std::shared_ptr<Commercial> commercial_45_6;
public:
    City(std::string name = "City");
    ~City() {}
};
}
#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_CITY_H_ */