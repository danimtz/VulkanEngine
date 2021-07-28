#pragma once

#include "Engine/Mesh/Mesh.h"
#include "Engine/Material/Material.h"

//Implementation is very temporary, needs a rework:
//This class holds a mesh and a material. This should be later replaced when an ECS is implemented. should be called
//mesh component or something the sort. Also for now holds a full copy for each mesh and material. This should be a pointer to the resource stored somewhere
//so that meshes and materials can be resued without occupying more memeory
class Model
{
public:

	Model(const char* mesh_filepath, const char* material_name, const char* material_filepath );

	std::shared_ptr<Mesh>	mesh() const { return m_mesh; };
	std::shared_ptr<Material> material() const { return m_material; };

private:

	std::shared_ptr<Mesh>	m_mesh;
	std::shared_ptr<Material> m_material;

};
