#pragma once

#include "Veist/Mesh/Mesh.h"
#include "Veist/Material/PBRMaterial.h"
#include "Veist/Util/AssetLoader.h"

//TODO: Refactor this entire class. only temporary.
//This class holds a mesh and a material. This should be later replaced when an ECS is implemented. should be called
//mesh component or something the sort. Also for now holds a full copy for each mesh and material. This should be a pointer to the resource stored somewhere
//so that meshes and materials can be resued without occupying more memeory

namespace Veist
{

class Model
{
public:

	Model(std::string folder_filepath, std::string material_name, std::string gltf_file);


	std::shared_ptr<Mesh>	mesh() const { return m_mesh; };
	std::shared_ptr<PBRMaterial> material() const { return m_material; };


	//temp
	glm::mat4& modelMatrix() const { return m_model_mat; };

private:

	std::shared_ptr<Mesh>	m_mesh;
	std::shared_ptr<PBRMaterial> m_material;

	//temp until ECS
	mutable glm::mat4 m_model_mat{1.0f};

};

}