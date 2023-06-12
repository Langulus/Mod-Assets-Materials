///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Scene.hpp"

using namespace Nodes;


/// Scene node as member constructor                                          
///   @param parent - the owning node                                         
///   @param desc - the node descriptor                                       
Scene::Scene(Node* parent, const Descriptor& desc)
   : Node {MetaOf<Scene>(), parent, desc} {}

/// Scene node descriptor-constructor                                         
///   @param desc - the node descriptor                                       
Scene::Scene(const Descriptor& desc)
   : Node {MetaOf<Scene>(), desc} {}

/// For logging                                                               
Scene::operator Debug() const {
   Code result;
   result += Node::DebugBegin();
      //result += pcSerialize<Debug>(mGeometry);
   result += Node::DebugEnd();
   return result;
}

/// Generate scene code                                                       
///   @return the SDF scene function template symbol                          
Symbol Scene::Generate() {
   Descend();
}