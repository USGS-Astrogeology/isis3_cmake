#include <iostream>
#include <cstdlib>
#include "LunarLambertMcEwen.h"
#include "PhotoModelFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "IException.h"
#include "Preference.h"

using namespace Isis;

int main() {
  Isis::Preference::Preferences(true);

  std::cout << "UNIT TEST for LunarLambertMcEwen photometric function" <<
            std::endl << std::endl;

  PvlGroup alg("Algorithm");
  alg += PvlKeyword("Name", "LunarLambertMcEwen");

  PvlObject o("PhotometricModel");
  o.addGroup(alg);

  Pvl pvl;
  pvl.addObject(o);
  std::cout << pvl << std::endl << std::endl;

  try {
    LunarLambertMcEwen *pm = (LunarLambertMcEwen *)PhotoModelFactory::Create(pvl);

    std::cout << "Test phase=0.0, incidence=0.0, emission=0.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(0.0, 0.0, 0.0) <<
              std::endl;
    std::cout << "Test phase=60.0, incidence=45.0, emission=30.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(60.0, 45.0, 30.0) <<
              std::endl;
    std::cout << "Test phase=180.0, incidence=90.0, emission=90.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(180.0, 90.0, 90.0) <<
              std::endl << std::endl;
  }
  catch(IException &e) {
    e.print();
  }

  return 0;
}
