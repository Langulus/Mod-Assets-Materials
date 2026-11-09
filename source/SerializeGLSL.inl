///                                                                           
/// Langulus::Module::Art::Materials                                          
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "GLSL.hpp"
#include <Langulus/CppAPI/Image.hpp>


/// Convert a block to GLSL code                                              
///   @param block the block to convert to GLSL                               
/*GLSL GLSL::FromScope(CT::Deep auto const& scope) {
   auto& dense_scope = DenseCast(scope);
   if (dense_scope.IsOr())
      throw Exception("GLSL doesn't support OR blocks");

   const auto count = dense_scope.GetCount();
   GLSL result;

   if (dense_scope.IsDeep()) {
      // Nest                                                           
      for (size_t i = 0; i < count; ++i) {
         result += FromScope(dense_scope.template AsAt<Many>(i));
         if (i < count - 1)
            result += ", ";
      }
      return result;
   }

   if (dense_scope.template CastsTo<Text>()) {
      // Contained type is code - just concatenate everything           
      for (size_t i = 0; i < count; ++i) {
         result += dense_scope.template AsAt<Text>(i);
         if (i < count - 1)
            result += ", ";
      }
      return result;
   }

   // Finally, attempt to interpret each element as GLSL and concat     
   TODO();
   return result;
}*/

/// Vector -> GLSL serializer                                                 
///   @param v vector to serialize                                            
GLSL GLSL::FromVector(CT::CustomVector auto const& v) {
   using V = Deref<decltype(v)>;
   if constexpr (ExtentOf<V> == 1)
      return GLSL {v[0]};
   else {
      GLSL result = FromType<V>();
      result += '(';
      for (size_t i = 0; i < ExtentOf<V>; ++i) {
         result += v[i];
         if (i < ExtentOf<V> - 1)
            result += ", ";
      }
      result += ')';
   }
}

/// Matrix -> GLSL serializer                                                 
///   @param m matrix to serialize                                            
GLSL GLSL::FromMatrix(CT::Matrix auto const& m) {
   using M = Deref<decltype(m)>;
   GLSL result = FromType<M>();
   result += '(';
   for (size_t i = 0; i < ExtentOf<M>; ++i) {
      result += m[i];
      if (i < ExtentOf<M> - 1)
         result += ", ";
   }
   result += ')';
}

/// Quaternion -> GLSL serializer                                             
///   @param q quaternion to serialize                                        
GLSL GLSL::FromQuaternion(CT::Quaternion auto const& q) {
   GLSL result = FromType<decltype(q)>();
   result += '(';
   result += q[0];
   result += ", ";
   result += q[1];
   result += ", ";
   result += q[2];
   result += ", ";
   result += q[3];
   result += ')';
   return result;
}

/// GLSL static tag string conversion                                         
///   @return the GLSL string                                                 
template<CT::DefineTag T> LANGULUS(INLINED)
GLSL GLSL::FromTag() {
   return FromTag(MetaTagOf<T>());
}

/// GLSL tag string conversion                                                
///   @param tag the tag                                                      
///   @return the GLSL string                                                 
inline GLSL GLSL::FromTag(TMeta tag) {
   return GLSL {"tag"} + tag.GetName();
}

/// GLSL static type string conversion                                        
///   @return the GLSL string                                                 
template<CT::NotVoid T> LANGULUS(INLINED)
GLSL GLSL::FromType() {
   return FromType(MetaDataOf<Decay<T>>());
}

/// GLSL type string conversion                                               
///   @param meta the type                                                    
///   @return the GLSL string                                                 
inline GLSL GLSL::FromType(DMeta meta) {
   meta = meta.GetMostConcrete();

   // Scalars, vectors, matrices, quaternions, all based on numbers.    
   // All we do here is decide the prefix, or early-return if scalar.   
   if (meta.CastsTo(MetaDataOf<Math::Number>())) {
      char prefix;
      size_t scalar_size;

      RTTI::DefinitionData::Base base;
      if (meta.GetBase(MetaDataOf<float>(), 0, base)) {
         scalar_size = sizeof(float);
         if (meta.GetSize() == sizeof(float))
            return "float";   // Scalar                                 
      }
      else if (meta.GetBase(MetaDataOf<double>(), 0, base)) {
         scalar_size = sizeof(double);
         if (meta.GetSize() == sizeof(double))
            return "double";  // Scalar                                 
         prefix = 'd';
      }
      else if (meta.GetBase(MetaDataOf<uint32_t>(), 0, base)) {
         scalar_size = sizeof(uint32_t);
         if (meta.GetSize() == sizeof(uint32_t))
            return "uint";    // Scalar                                 
         prefix = 'u';
      }
      else if (meta.GetBase(MetaDataOf<int32_t>(), 0, base)) {
         scalar_size = sizeof(int32_t);
         if (meta.GetSize() == sizeof(int32_t))
            return "int";     // Scalar                                 
         prefix = 'i';
      }
      else if (meta.GetBase(MetaDataOf<bool>(), 0, base)) {
         scalar_size = sizeof(bool);
         if (meta.GetSize() == sizeof(bool))
            return "bool";    // Scalar                                 
         prefix = 'b';
      }
      else throw Exception("Unsupported GLSL type");

      //                                                                
      // If this is reached, then we have a vector/matrix/quaternion    
      Text token = prefix;
      if (meta.CastsTo(MetaDataOf<Math::Matrix>())) {
         // Find the number of columns in the matrix                    
         size_t cols = 0;
         size_t rows = 0;

         RTTI::DefinitionData::Base matbase;
         if (meta.GetBase(MetaDataOf<Math::MatrixOfColumns<1>>(), 0, matbase)) {
            // Single column, so return as vector                       
            token += "vec";
            token += meta.GetSize() / scalar_size;
            return token;
         }
         else if (meta.GetBase(MetaDataOf<Math::MatrixOfColumns<2>>(), 0, matbase))
            cols = 2;
         else if (meta.GetBase(MetaDataOf<Math::MatrixOfColumns<3>>(), 0, matbase))
            cols = 3;
         else if (meta.GetBase(MetaDataOf<Math::MatrixOfColumns<4>>(), 0, matbase))
            cols = 4;
         else throw Exception("Matrix has too many columns");

         // If reached, then we have a matrix                           
         rows = meta.GetSize() / (cols * scalar_size);
         token += "mat";
         if (cols == rows) {
            // Square matrix, so only one suffix number                 
            token += cols;
         }
         else {
            // Non-square matrix, so two suffix numbers                 
            token += cols;
            token += 'x';
            token += rows;
         }
      }
      else if (meta.CastsTo(MetaDataOf<Math::Vector>())) {
         token += "vec";
         token += meta.GetSize() / scalar_size;
      }
      else throw Exception("Unsupported GLSL type");

      return token;
   }
   else if (meta.CastsTo(MetaDataOf<Things::Image>())) {
      return "sampler2D";
      //TODO distinguish these:
      //gsampler1D   GL_TEXTURE_1D   1D texture
      //gsampler2D   GL_TEXTURE_2D   2D texture
      //gsampler3D   GL_TEXTURE_3D   3D texture
      //gsamplerCube   GL_TEXTURE_CUBE_MAP   Cubemap Texture
      //gsampler2DRect   GL_TEXTURE_RECTANGLE   Rectangle Texture
      //gsampler1DArray   GL_TEXTURE_1D_ARRAY   1D Array Texture
      //gsampler2DArray   GL_TEXTURE_2D_ARRAY   2D Array Texture
      //gsamplerCubeArray   GL_TEXTURE_CUBE_MAP_ARRAY   Cubemap Array Texture (requires GL 4.0 or ARB_texture_cube_map_array)
      //gsamplerBuffer   GL_TEXTURE_BUFFER   Buffer Texture
      //gsampler2DMS   GL_TEXTURE_2D_MULTISAMPLE   Multisample Texture
      //gsampler2DMSArray   GL_TEXTURE_2D_MULTISAMPLE_ARRAY   Multisample Array Texture
   }

   throw Exception("Unsupported GLSL type");
}