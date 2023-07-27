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

/// Flow code for generating a complex material                               
constexpr auto MaterialCode = R"code(
   Nodes::Scene(
      // Create an animated rectangle, that rotates/scales/moves with time
      Box2, [
         Interpolator(Cerp), Time(.Time % 5),
         Move @0   (Yaw(180), Scale(275.5, 381), Point(400, 400)),
         Move @1   (Roll(3),  Scale(551, 762),   .MousePosition),
         Move @1.5 (Roll(3),  Scale(551, 762),   .MousePosition),
         Move @3   (          Scale(551, 762),   .MousePosition),
         Move @4   (Yaw(180), Scale(275.5, 381), Point(400, 400))
      ]
   ),

   // Rasterize scene. Since no scene was provided as argument, 
   // it will seek an available Nodes::Scene in the node hierarchy
   Nodes::Raster(Bilateral),

   // Texturize the scene. Since no node was provided as argument,
   // it will seek an available Nodes::Raster in the node hierarchy
   Nodes::Texture(Front, `pebbles.png`),
   Nodes::Texture(Back,  `border2.png`),
   Nodes::Texture([
      Nodes::FBM(4, [vec2(.Sampler.x, -(.Time * 8.75 - .Sampler.y ^ 2)) rand real])
   ]),

   // Illuminate directionally. Since no node was provided as argument,
   // it will seek an available Nodes::Raster in the node hierarchy
   Nodes::Light(Normal3(0.5))
)code";


SCENARIO("Shader generation", "[materials]") {
   for (int repeat = 0; repeat != 10; ++repeat) {
      GIVEN(std::string("Init and shutdown cycle #") + std::to_string(repeat)) {
         // Create root entity                                          
         Thing root;
         root.SetName("ROOT");

         // Create runtime at the root                                  
         root.CreateRuntime();

         // Load modules                                                
         root.LoadMod("AssetsMaterials");

         WHEN("The material is created via tokens") {
            auto producedMaterial = root.CreateUnitToken("Material", Code(MaterialCode));
            
            // Update once                                              
            root.Update(Time::zero());
            
            THEN("Various traits change") {
               root.DumpHierarchy();

               REQUIRE(producedMaterial.GetCount() == 1);
               REQUIRE(producedMaterial.CastsTo<A::Material>(1));
               REQUIRE(producedMaterial.IsSparse());
            }
         }

         WHEN("The material is created via abstractions") {
            auto producedMaterial = root.CreateUnit<A::Material>(Code(MaterialCode));

            // Update once                                              
            root.Update(Time::zero());

            THEN("Various traits change") {
               root.DumpHierarchy();

               REQUIRE(producedMaterial.GetCount() == 1);
               REQUIRE(producedMaterial.CastsTo<A::Material>(1));
               REQUIRE(producedMaterial.IsSparse());
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

