///                                                                           
/// Langulus::Module::Assets::Materials                                       
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
GLSL::GLSL(const CT::Deep auto& block) : Text {} {
   if (block.IsOr())
      throw Exception("GLSL doesn't support OR blocks");

   const auto count = block.GetCount();
   if (block.IsDeep()) {
      // Nest                                                           
      for (size_t i = 0; i < count; ++i) {
         *this += GLSL {block.template As<Block>(i)};
         if (i < count - 1)
            *this += ", ";
      }
      return;
   }

   if (block.template CastsTo<Text>()) {
      // Contained type is code - just concatenate everything           
      for (size_t i = 0; i < count; ++i) {
         *this += block.template As<Text>(i);
         if (i < count - 1)
            *this += ", ";
      }
      return;
   }

   // Finally, attempt to interpret each element as GLSL and concat     
   for (size_t index = 0; index < count; ++index) {
      *this = Verbs::Interpret::To<GLSL>(block);

      // Failed, but try doing GASM interpretation                      
      // It works most of the time :)                                   
      //TODO
   }
}

/// Vector -> GLSL serializer                                                 
///   @param vector vector to serialize                                       
template<CT::Number T, size_t C> LANGULUS(INLINED)
GLSL::GLSL(const TVector<T, C>& vector) {
   if constexpr (C == 1)
      *this += vector[0];
   else {
      *this += GLSL::template Type<TVector<T, C>>();
      *this += '(';
      for (size_t i = 0; i < C; ++i) {
         *this += vector[i];
         if (i < C - 1)
            *this += ", ";
      }
      *this += ')';
   }
}

/// Matrix -> GLSL serializer                                                 
///   @param matrix matrix to serialize                                       
template<CT::Number T, size_t C, size_t R> LANGULUS(INLINED)
GLSL::GLSL(const TMatrix<T, C, R>& matrix) {
   *this += GLSL::template Type<TMatrix<T, C, R>>();
   *this += '(';
   for (size_t i = 0; i < C * R; ++i) {
      *this += matrix[i];
      if (i < (C * R) - 1)
         *this += ", ";
   }
   *this += ')';
}

/// Quaternion -> GLSL serializer                                             
///   @param quaternion quaternion to serialize                               
template<CT::Number T> LANGULUS(INLINED)
GLSL::GLSL(const TQuaternion<T>& quaternion) {
   *this += GLSL::template Type<TQuaternion<T>>();
   *this += '(';
   *this += quaternion[0];
   *this += ", ";
   *this += quaternion[1];
   *this += ", ";
   *this += quaternion[2];
   *this += ", ";
   *this += quaternion[3];
   *this += ')';
}

/// GLSL static type string conversion                                        
///   @return the GLSL string                                                 
template<CT::NotVoid T> LANGULUS(INLINED)
GLSL GLSL::Type() {
   return Type(MetaOf<Decay<T>>());
}

/// GLSL type string conversion                                               
///   @param meta the type                                                    
///   @return the GLSL string                                                 
inline GLSL GLSL::Type(DMeta meta) {
   meta = meta.GetMostConcrete();

   // Scalars, vectors, matrices, quaternions, all based on numbers.    
   // All we do here is decide the prefix, or early-return if scalar.   
   if (meta.CastsTo<Math::Number>()) {
      char prefix;
      size_t scalar_size;

      RTTI::DefinitionData::Base base;
      if (meta.GetBase<float>(0, base)) {
         scalar_size = sizeof(float);
         if (meta.GetSize() == sizeof(float))
            return "float";   // Scalar                                 
      }
      else if (meta.GetBase<double>(0, base)) {
         scalar_size = sizeof(double);
         if (meta.GetSize() == sizeof(double))
            return "double";  // Scalar                                 
         prefix = 'd';
      }
      else if (meta.GetBase<uint32_t>(0, base)) {
         scalar_size = sizeof(uint32_t);
         if (meta.GetSize() == sizeof(uint32_t))
            return "uint";    // Scalar                                 
         prefix = 'u';
      }
      else if (meta.GetBase<int32_t>(0, base)) {
         scalar_size = sizeof(int32_t);
         if (meta.GetSize() == sizeof(int32_t))
            return "int";     // Scalar                                 
         prefix = 'i';
      }
      else if (meta.GetBase<bool>(0, base)) {
         scalar_size = sizeof(bool);
         if (meta.GetSize() == sizeof(bool))
            return "bool";    // Scalar                                 
         prefix = 'b';
      }
      else throw Exception("Unsupported GLSL type");

      //                                                                
      // If this is reached, then we have a vector/matrix/quaternion    
      Text token = prefix;
      if (meta.CastsTo<Math::Matrix>()) {
         // Find the number of columns in the matrix                    
         size_t cols = 0;
         size_t rows = 0;

         RTTI::DefinitionData::Base matbase;
         if (meta.GetBase<Math::MatrixOfColumns<1>>(0, matbase)) {
            // Single column, so return as vector                       
            token += "vec";
            token += meta.GetSize() / scalar_size;
            return token;
         }
         else if (meta.GetBase<Math::MatrixOfColumns<2>>(0, matbase))
            cols = 2;
         else if (meta.GetBase<Math::MatrixOfColumns<3>>(0, matbase))
            cols = 3;
         else if (meta.GetBase<Math::MatrixOfColumns<4>>(0, matbase))
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
      else if (meta.CastsTo<Math::Vector>()) {
         token += "vec";
         token += meta.GetSize() / scalar_size;
      }
      else throw Exception("Unsupported GLSL type");

      return token;
   }
   else if (meta.CastsTo<Things::Image>()) {
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