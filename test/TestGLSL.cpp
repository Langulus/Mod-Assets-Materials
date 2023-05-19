///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Main.hpp"
#include <Entity/Thing.hpp>
#include <catch2/catch.hpp>

#if LANGULUS_FEATURE(MEMORY_STATISTICS)
static bool statistics_provided = false;
static Anyness::Allocator::Statistics memory_statistics;
#endif

/// See https://github.com/catchorg/Catch2/blob/devel/docs/tostring.md        
CATCH_TRANSLATE_EXCEPTION(::Langulus::Exception const& ex) {
   const Text serialized {ex};
   return ::std::string {Token {serialized}};
}

SCENARIO("Shader generation", "[materials]") {
   for (int repeat = 0; repeat != 10; ++repeat) {
      GIVEN(std::string("Init and shutdown cycle #") + std::to_string(repeat)) {
         // Create root entity                                          
         Thing root;
         root.SetName("ROOT"_text);

         // Create runtime at the root                                  
         root.CreateRuntime();

         // Load ImGui module                                           
         root.LoadMod("AssetsMaterials");

         WHEN("The system is created via tokens") {
            TODO();

            // Update once                                              
            root.Update(Time::zero());

            THEN("Various traits change") {
               root.DumpHierarchy();

               REQUIRE(true);
            }
         }
                  
         #if LANGULUS_FEATURE(MEMORY_STATISTICS)
            Fractalloc.CollectGarbage();

            // Detect memory leaks                                      
            if (statistics_provided) {
               if (memory_statistics != Fractalloc.GetStatistics()) {
                  Fractalloc.DumpPools();
                  memory_statistics = Fractalloc.GetStatistics();
                  FAIL("Memory leak detected");
               }
            }

            memory_statistics = Fractalloc.GetStatistics();
            statistics_provided = true;
         #endif
      }
   }
}

