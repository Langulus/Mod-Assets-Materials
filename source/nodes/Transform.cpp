///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Transform.hpp"

using namespace Nodes;


/// Transformation node descriptor-constructor                                
///   @param desc - the node descriptor                                       
Transform::Transform(const Descriptor& desc)
   : Node {MetaOf<Transform>(), desc} { }

/// For logging                                                               
Transform::operator Debug() const {
   Code result;
   result += Node::DebugBegin();
      //result += mKeyframes;
   result += Node::DebugEnd();
   return result;
}

/// Move/rotate/scale instance                                                
///   @param verb - the move verb                                             
/*void Transform::Move(Verb& verb) {
   const auto time = PCTime::FromSeconds(verb.GetTime());
   const auto found = mKeyframes.FindKey(time);
   if (!found) {
      // Create new keyframe                                            
      mKeyframes.Add(time, verb);
      VERBOSE_NODE("Transformation keyframe added: " << verb);
      verb << this;
      return;
   }

   // Combine the keyframes                                             
   mKeyframes[time].GetArgument() << verb.GetArgument();
   verb << this;
}*/

/// Get a transformation matrix from a specific keyframe                      
///   @param idx - the keyframe index                                         
///   @return the instance                                                    
TInstance<Vec3> Transform::GetInstance(Offset idx) {
   auto keyframe = mKeyframes.GetValue(idx);
   //bool relative = IsRelativeKeyframe(keyframe);
   TInstance<Vec3> instance;
   instance.Move(keyframe);
   return instance;
}

/// Turn a keyframe position to GLSL code                                     
///   @param idx - the index of the keyframe                                  
///   @return the generated GLSL code                                         
GLSL Transform::GetPosition(Offset idx, bool& runtime) {
   GLSL symbol;
   auto& keyframe = mKeyframes.GetValue(idx);
   keyframe.ForEachDeep([&](const Block& group) {
      group.ForEach([&](const MaterialNode& input) {
         symbol = input.GetOutputSymbol();
         runtime = true;
      });
      group.ForEach([&](const Construct& construct) {
         symbol = CodeFromConstruct(construct);
         runtime = true;
      });
   });

   if (symbol.IsEmpty()) {
      const auto instance = GetInstance(idx);
      symbol += instance.GetPosition();
   }

   return symbol;
}

/// Turn a keyframe scale to GLSL code                                        
///   @param idx - the index of the keyframe                                  
///   @return the generated GLSL code                                         
GLSL Transform::GetScale(Offset idx, bool&) {
   const auto instance = GetInstance(idx);
   GLSL result;
   result += instance.GetScale();
   return result;
}

/// Turn a keyframe orientation to GLSL code                                  
///   @param idx - the index of the keyframe                                  
///   @return the generated GLSL code                                         
GLSL Transform::GetAim(Offset idx, bool&) {
   const auto instance = GetInstance(idx);
   GLSL result;
   result += instance.GetAim();
   return result;
}

/// Turn a keyframe interpolator to GLSL code                                 
///   @param idx - the index of the keyframe                                  
///   @return the generated GLSL code                                         
GLSL Transform::GetInterpolator(Offset idx) {
   // Scan for interpolator traits up to the requested keyframe idx     
   auto interpolator = Verbs::Lerp::ID;
   Offset progress = 0;
   for (const auto& keyframe : mKeyframes.Values()) {
      keyframe.ForEachDeep([&](const Block& group) {
         group.ForEach([&](const Trait& trait) {
            if (trait.TraitIs<Traits::Interpolator>())
               interpolator = trait.AsCast<VerbID>();
         });
      });

      if (idx == progress)
         break;
      ++progress;
   }

   // Generate the code                                                 
   GLSL result;
   result += interpolator;
   return result;
}

/// Turn a keyframe time to GLSL code                                         
///   @param idx - the index of the keyframe                                  
///   @return the generated GLSL code                                         
GLSL Transform::GetTimer(Offset idx) {
   // Scan for interpolator traits up to the requested keyframe idx     
   Any timer;
   pcptr progress = 0;
   for (const auto& keyframe : mKeyframes.Values()) {
      keyframe.ForEachDeep([&](const Block& group) {
         group.ForEach([&](const Trait& trait) {
            if (trait.TraitIs<Traits::Time>())
               timer = static_cast<const Any&>(trait);
         });
      });

      if (idx == progress)
         break;
      ++progress;
   }

   return CodeFromScope(timer);
}

/// Generate vec4 Transform(vec3) function                                    
void Transform::GenerateDefinition() {
   // Generate the function body                                        
   GLSL define;
   if (mKeyframes.IsEmpty()) {
      // No transformations were applied to the instance                
      define = 
         "vec4 Transform(in vec3 point) {\n"
         "   return vec4(point, 1.0);\n"
         "}\n\n";

      Commit(ShaderToken::Functions, define);
      return;
   }
   else if (mKeyframes.GetCount() == 1){
      // Single keyframe early return                                   
      const auto model = GetInstance(0).GetModelTransform(Level::Default);
      define = 
         "vec4 Transform(in vec3 point) {\n"
         "   const mat4 model = "_glsl + model + ";\n"
         "   return model * vec4(point, 1.0);\n"
         "}\n\n";

      Commit(ShaderToken::Functions, define);
      return;
   }

   // Multiple transformations found - generate keyframes               
   // 1. Collect time offsets                                           
   const auto count = mKeyframes.GetCount();
   GLSL keyframeTime 
      = "const float cKeyframeTime["_glsl + count + "] = float["
      + count + "](";

   const auto animationStart = mKeyframes.GetKey(0).SecondsReal();
   const auto animationEnd = mKeyframes.Keys().Last().SecondsReal();
   mKeyframes.ForEach([&](const PCTime& time, const Verb& data) {
      keyframeTime += time.SecondsReal();
      if (&data != &mKeyframes.Values().Last())
         keyframeTime += ", ";
      return true;
   });
   keyframeTime += ");\n\n";

   // 2. Collect interpolators                                          
   GLSL keyframeInterpolate 
      = "#define Lerp 0\n"_glsl
      + "#define Cerp 1\n"
      + "const int cKeyframeInterpolator[" + count + "] = int["
      + count + "](";

   SuccessTrap interpolatorDynamic;
   SuccessTrap cubicDependencies;
   const auto interpolatorReference = GetInterpolator(0);
   for (pcptr i = 0; i < mKeyframes.GetCount(); ++i) {
      const auto interpolator = GetInterpolator(i);
      interpolatorDynamic = interpolatorReference != interpolator;
      keyframeInterpolate += interpolator;
      cubicDependencies = interpolator == "Cerp";
      if (i < mKeyframes.GetCount() - 1)
         keyframeInterpolate += ", ";
   }
   keyframeInterpolate += ");\n\n";

   // 3. Collect timers                                                 
   auto timerCode = GetTimer(0);

   // 4. Collect positions                                              
   GLSL keyframePosition 
      = "vec3 cKeyframePosition["_glsl + count + "] = vec3["
      + count + "](\n";

   SuccessTrap positionDynamic;
   bool positionRuntime = false;
   const auto positionReference = GetPosition(0, positionRuntime);
   for (pcptr i = 0; i < mKeyframes.GetCount(); ++i) {
      const auto position = GetPosition(i, positionRuntime);
      positionDynamic = positionReference != position;
      keyframePosition += "   ";
      keyframePosition += position;
      if (i < mKeyframes.GetCount() - 1)
         keyframePosition += ", \n";
   }
   keyframePosition += "\n);\n\n";

   if (!positionRuntime) {
      // We can afford position keyframes as constexpr                  
      keyframePosition = "const " + keyframePosition;
   }

   // 5. Collect scale                                                  
   GLSL keyframeSize 
      = "vec3 cKeyframeScale["_glsl + count + "] = vec3["
      + count + "](\n";

   SuccessTrap sizeDynamic;
   bool sizeRuntime = false;
   const auto sizeReference = GetScale(0, sizeRuntime);
   for (Offset i = 0; i < mKeyframes.GetCount(); ++i) {
      const auto size = GetScale(i, sizeRuntime);
      sizeDynamic = sizeReference != size;
      keyframeSize += "   ";
      keyframeSize += size;
      if (i < mKeyframes.GetCount() - 1)
         keyframeSize += ", \n";
   }
   keyframeSize += "\n);\n\n";

   if (!sizeRuntime) {
      // We can afford scale keyframes as constexpr                     
      keyframeSize = "const " + keyframeSize;
   }

   // 6. Collect orientation                                            
   GLSL keyframeAim 
      = "vec4 cKeyframeAim["_glsl + count + "] = vec4["
      + count + "](\n";

   SuccessTrap aimDynamic;
   bool aimRuntime = false;
   const auto aimReference = GetAim(0, aimRuntime);
   for (Offset i = 0; i < mKeyframes.GetCount(); ++i) {
      const auto aim = GetAim(i, aimRuntime);
      aimDynamic = aimReference != aim;
      keyframeAim += "   ";
      keyframeAim += aim;
      if (i < mKeyframes.GetCount() - 1)
         keyframeAim += ", \n";
   }
   keyframeAim += "\n);\n\n";

   if (!aimRuntime) {
      // We can afford aim keyframes as constexpr                       
      keyframeAim = "const " + keyframeAim;
   }

   // 7. Check if no transformation is dynamic                          
   if (!positionDynamic && !sizeDynamic && !aimDynamic) {
      // All applied transformations are the same                       
      // Single keyframe early return                                   
      const auto model = GetInstance(0).GetModelTransform(Level::Default);
      define += 
         "vec4 Transform(in vec3 point) {\n"
         "   const mat4 model = "_glsl + model + ";\n"
         "   return model * vec4(point, 1.0);\n"
         "}\n\n";
      Commit(ShaderToken::Functions, define);
      return;
   }

   // 8. Write constants                                                
   // Animation limits                                                  
   define += 
      "const int cKeyframeCount = "_glsl + count + ";\n"
      "const float cAnimationStart = " + animationStart + ";\n"
      "const float cAnimationEnd = " + animationEnd + ";\n"
      "const float cAnimationLength = " + (animationEnd - animationStart) + ";\n\n";

   // Time offsets                                                      
   define += keyframeTime;

   // Dynamic interpolators, if any                                     
   if (interpolatorDynamic)
      define += keyframeInterpolate;

   // Dynamic positions, if any                                         
   if (positionDynamic)
      define += keyframePosition;

   // Dynamic size, if any                                              
   if (sizeDynamic)
      define += keyframeSize;

   // Dynamic aim, if any                                               
   if (aimDynamic)
      define += keyframeAim;

   // Function that generates a 4x4 matrix or vec4 from a keyframe      
   {
      define += aimDynamic || sizeDynamic ? "mat4 " : "vec4 ";
      define += "ComposeKeyframe(";
      if (positionDynamic) {
         define += "in vec4 mooved";
         if (sizeDynamic || aimDynamic)
            define += ", ";
      }
      if (sizeDynamic) {
         define += "in vec4 scaled";
         if (aimDynamic)
            define += ", ";
      }
      if (aimDynamic) {
         define += "in vec4 orient";
      }
      define += ") {\n";

      const bool relevantPosition = positionDynamic || positionReference != "vec3(0, 0, 0)";
      if (!positionDynamic && relevantPosition)
         define += "const vec4 mooved = vec4(" + positionReference + ", 0.0);\n";

      const bool relevantSize = sizeDynamic || sizeReference != "vec3(1, 1, 1)";
      if (!sizeDynamic && relevantSize)
         define += "const vec4 scaled = vec4(" + sizeReference + ", 1.0);\n";

      const bool relevantAim = aimDynamic || aimReference != "vec4(0, 0, 0, 1)";
      if (!aimDynamic && relevantAim)
         define += "const vec4 orient = normalize(" + aimReference + ");\n";

      if (relevantAim) {
         define +=
            "   const vec4 orient2 = orient * 2.0;\n"
            "   float xx = orient.x * orient2.x;\n"
            "   float xy = orient.x * orient2.y;\n"
            "   float xz = orient.x * orient2.z;\n"
            "   float yy = orient.y * orient2.y;\n"
            "   float yz = orient.y * orient2.z;\n"
            "   float zz = orient.z * orient2.z;\n"
            "   float wx = orient.w * orient2.x;\n"
            "   float wy = orient.w * orient2.y;\n"
            "   float wz = orient.w * orient2.z;\n\n";
      }

      if (relevantAim || relevantSize) {
         define +=
            "   return mat4(\n"
            "      // Column 0\n";

         // We're animating via a matrix                                
         if (relevantAim && relevantSize) {
            define +=
               "      vec4(1.0 - (yy + zz), xy - wz, xz + wy, 0.0) * scaled, \n"
               "      // Column 1\n"
               "      vec4(xy + wz, 1.0 - (xx + zz), yz - wx, 0.0) * scaled, \n"
               "      // Column 2\n"
               "      vec4(xz - wy, yz + wx, 1.0 - (xx + yy), 0.0) * scaled, \n";
         }
         else if (relevantAim) {
            define +=
               "      vec4(1.0 - (yy + zz), xy - wz, xz + wy, 0.0), \n"
               "      // Column 1\n"
               "      vec4(xy + wz, 1.0 - (xx + zz), yz - wx, 0.0), \n"
               "      // Column 2\n"
               "      vec4(xz - wy, yz + wx, 1.0 - (xx + yy), 0.0), \n";
         }
         else if (relevantSize) {
            define +=
               "      vec4(scaled.x, 0.0, 0.0, 0.0), \n"
               "      // Column 1\n"
               "      vec4(0.0, scaled.y, 0.0, 0.0), \n"
               "      // Column 2\n"
               "      vec4(0.0, 0.0, scaled.z, 0.0), \n";
         }

         if (relevantPosition) {
            define +=
               "      // Column 3\n"
               "      vec4(mooved.xyz, 1.0) \n";
         }
         else {
            define +=
               "      // Column 3\n"
               "      vec4(0.0, 0.0, 0.0, 1.0) \n";
         }

         define += "   );\n";
      }
      else {
         // We're animating via a simple position offset                
         define += "   return mooved;\n";
      }

      define += "}\n\n";
   }

   // Some general purpose cubic interpolators                          
   if (cubicDependencies) {
      static const GLSL cerps =
         "float cerp(in float n0, in float n1, in float n2, in float n3, in float a) {\n"
         "   const float t2 = a * a;\n"
         "   const float t3 = t2 * a;\n"
         "   const float p = (n3 - n2) - (n0 - n1);\n"
         "   return p * t3 + ((n0 - n1) - p) * t2 + (n2 - n0) * a + n1;\n"
         "}\n\n"

         "vec3 cerp(in vec3 n0, in vec3 n1, in vec3 n2, in vec3 n3, in float a) {\n"
         "   const float t2 = a * a;\n"
         "   const float t3 = t2 * a;\n"
         "   const vec3 p = (n3 - n2) - (n0 - n1);\n"
         "   return p * t3 + ((n0 - n1) - p) * t2 + (n2 - n0) * a + n1;\n"
         "}\n\n"

         "vec4 cerp(in vec4 n0, in vec4 n1, in vec4 n2, in vec4 n3, in float a) {\n"
         "   const float t2 = a * a;\n"
         "   const float t3 = t2 * a;\n"
         "   const vec4 p = (n3 - n2) - (n0 - n1);\n"
         "   return p * t3 + ((n0 - n1) - p) * t2 + (n2 - n0) * a + n1;\n"
         "}\n\n";

      define += cerps;
   }

   // Function that interpolates between two keyframes                  
   {
      define += aimDynamic || sizeDynamic ? "mat4 " : "vec4 ";
      define +=
         "InterpolateKeyframes(in int keyStart, in int keyEnd, in float time) {\n"
         "   const float ratio = (time - cKeyframeTime[keyStart]) / (cKeyframeTime[keyEnd] - cKeyframeTime[keyStart]);\n";

      if (interpolatorDynamic) {
         TODO();
      }
      else if (interpolatorReference == "Cerp") {
         define += "   const int prev = keyStart > 0 ? keyStart - 1 : 0;\n";
         define += "   const int next = keyEnd < cKeyframeCount - 1 ? keyEnd + 1 : cKeyframeCount - 1;\n\n";

         if (positionDynamic)
            define += "   const vec4 mooved = vec4(cerp(cKeyframePosition[prev], cKeyframePosition[keyStart], cKeyframePosition[keyEnd], cKeyframePosition[next], ratio), 0.0);\n";
         if (sizeDynamic)
            define += "   const vec4 scaled = vec4(cerp(cKeyframeScale[prev], cKeyframeScale[keyStart], cKeyframeScale[keyEnd], cKeyframeScale[next], ratio), 1.0);\n";
         if (aimDynamic)
            define += "   const vec4 orient = cerp(cKeyframeAim[prev], cKeyframeAim[keyStart], cKeyframeAim[keyEnd], cKeyframeAim[next], ratio);\n";
      }
      else if (interpolatorReference == "Lerp") {
         if (positionDynamic)
            define += "   const vec4 mooved = vec4(mix(cKeyframePosition[keyStart], cKeyframePosition[keyEnd], ratio), 0.0);\n";
         if (sizeDynamic)
            define += "   const vec4 scaled = vec4(mix(cKeyframeScale[keyStart], cKeyframeScale[keyEnd], ratio), 1.0);\n";
         if (aimDynamic)
            define += "   const vec4 orient = mix(cKeyframeAim[keyStart], cKeyframeAim[keyEnd], ratio);\n";
      }
      else TODO();

      define += "   return ComposeKeyframe(";
      if (positionDynamic) {
         define += "mooved";
         if (sizeDynamic || aimDynamic)
            define += ", ";
      }
      if (sizeDynamic) {
         define += "scaled";
         if (aimDynamic)
            define += ", ";
      }
      if (aimDynamic) {
         define += "normalize(orient)";
      }

      define += ");\n}\n\n";
   }

   // The animate function picks the keyframes depending on time        
   {
      define += aimDynamic || sizeDynamic ? "mat4 " : "vec4 ";
      define +=
         "Animate(in float time) {\n"
         "   if (time <= cAnimationStart) {\n"
         "      // Time before/at start returns first keyframe\n"
         "      return ComposeKeyframe(";

      if (positionDynamic) {
         define += "vec4(cKeyframePosition[0], 0.0)";
         if (sizeDynamic || aimDynamic)
            define += ", ";
      }
      if (sizeDynamic) {
         define += "vec4(cKeyframeScale[0], 1.0)";
         if (aimDynamic)
            define += ", ";
      }
      if (aimDynamic) {
         define += "cKeyframeAim[0]";
      }

      define +=
         ");\n   }\n"
         "   else if (time >= cAnimationEnd) {\n"
         "      // Time at/after last keyframe returns last keyframe\n"
         "      const int i = cKeyframeCount - 1;\n"
         "      return ComposeKeyframe(";

      if (positionDynamic) {
         define += "vec4(cKeyframePosition[i], 0.0)";
         if (sizeDynamic || aimDynamic)
            define += ", ";
      }
      if (sizeDynamic) {
         define += "vec4(cKeyframeScale[i], 1.0)";
         if (aimDynamic)
            define += ", ";
      }
      if (aimDynamic) {
         define += "cKeyframeAim[i]";
      }

      define +=
         ");\n   }\n\n"
         "   for (int key = 0; key < cKeyframeCount - 1; key += 1) {\n"
         "      if (time >= cKeyframeTime[key] && time <= cKeyframeTime[key + 1]) {\n"
         "         // Interpolate between two frames\n"
         "         return InterpolateKeyframes(key, key + 1, time);\n"
         "      }\n"
         "   }\n\n";

      if (aimDynamic || sizeDynamic)
         define += "   return mat4(1.0);\n";
      else
         define += "   return vec4(0.0);\n";

      define += "}\n\n";
   }

   // The transform function applies the interpolated keyframe          
   {
      define += "vec4 Transform(in vec3 point) {\n   return ";
      if (timerCode.IsEmpty()) {
         auto time = GetSymbol<Traits::Time>(RRate::PerTick);
         define += "Animate(" + time + ")";
      }
      else define += "Animate(" + timerCode + ")";
      define += aimDynamic || sizeDynamic ? " * " : " + ";
      define += "vec4(point, 1.0);\n}\n\n";
   }

   Commit(ShaderToken::Functions, define);
}

/// Generate the shader stages                                                
Symbol Transform::Generate() {
   Descend();
}
