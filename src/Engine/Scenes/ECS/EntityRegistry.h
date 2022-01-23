#pragma once

#include "Engine/Scenes/ECS/ECSTypes.h"
#include "Engine/Scenes/ECS/ComponentPool.h"
#include "Engine/Scenes/ECS/View.h"

namespace ecs{





class EntityRegistry
{
public:

	EntityRegistry();

	//Move constructor
	EntityRegistry(EntityRegistry&& other) 
		: m_entity_count{ std::move(other.m_entity_count) },
		m_free_entities{ std::move(other.m_free_entities) },
		m_entity_signatures{ std::move(other.m_entity_signatures) },
		m_component_pools{ std::move(other.m_component_pools)/*TODO: Test this*/ }{}
	
	//Move assignment operator 
	EntityRegistry &operator=(EntityRegistry&& other)
	{
		m_entity_count =  std::move(other.m_entity_count);
		m_free_entities = std::move(other.m_free_entities);
		m_entity_signatures = std::move(other.m_entity_signatures);
		m_component_pools = std::move(other.m_component_pools); //TODO: Test this
	}

	~EntityRegistry() = default;



	EntityId createEntity();


	void destroyEntity(EntityId entity);



	/*
	/ 	emplaceComponent()
	/ 	Adds a component to the entity given.
	/
	*/
	template<typename T, typename... Args>
	T& emplaceComponent(EntityId id, Args&&... args)//TODO recursive default constructor emplaceComponents <component1, component2, etc>
	{
		//Add and get component
		auto& component = findOrCreateComponentPool<T>()->addComponent(id, std::forward<Args>(args)...);

		//Add component to entity signature
		Signature& signature = getEntitySignature(id);
		signature.set( getComponentId<T>() ,true);

		return component;
	}

	/*
	/ 	removeComponent()
	/	Removes a component from the given entity
	/
	*/
	template<typename T>
	void removeComponent(EntityId id)//TODO recursive removeComponents <component1, component2, etc>
	{
		
		findOrCreateComponentPool<T>()->removeComponent(id);
		Signature& signature = getEntitySignature(id);
		signature.set(getComponentId<T>(), false);

	}


	/*
	/ 	view()
	/	Creates a view of the registry giving back an iteratable object containing all the entities 
	/	that contain the components requested
	/
	*/
	template<typename... ComponentTypes>
	View view()
	{
		Signature signature;
		if (sizeof...(ComponentTypes) == 0)
		{
			//Set entity signature to all true
			signature.set();
		}
		else
		{
			//Unpack template parameters into an initializer list, 0 needed if empty
			int component_ids[] = { 0, getComponentId<ComponentTypes>() ... };
			for (int i = 1; i < (sizeof...(ComponentTypes) + 1 ); i++)
			{
				signature.set(component_ids[i]);
			}
		}

		return {signature,  &m_entities, &m_entity_signatures, &m_component_pools };
	}


private:
	
	
	

	Signature& getEntitySignature(EntityId id)
	{
		return m_entity_signatures[id];
	}

	


	template<typename T>
	ComponentPool<T>* findOrCreateComponentPool()
	{
		ComponentId component_id = getComponentId<T>();
		if (component_id >= m_component_pools.size())
		{
			m_component_pools.push_back(std::make_shared<ComponentPool<T>>() );
			assert(component_id == (m_component_pools.size()-1));

			return static_cast<ComponentPool<T>*>(m_component_pools[component_id].get());
		}
		else
		{
			return static_cast<ComponentPool<T>*>(m_component_pools[component_id].get());
		}
	}


	
	


private:

	//number of live entities
	uint32_t m_entity_count;

	//queue of unused entity ids
	std::queue<EntityId> m_free_entities;

	//active entities
	std::vector<EntityId> m_entities;

	//array of entity signatures. array index is the entity id
	std::array<Signature, MAX_ENTITIES> m_entity_signatures;

	std::vector<std::shared_ptr<ComponentPoolBase>> m_component_pools;

	
};


};