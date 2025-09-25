#ifndef ECS_H
#define ECS_H

#include <stdint.h>

#define CACHE_SIZE 64

typedef uint32_t id_t;
typedef uint8_t comp_id_t;
typedef uint8_t arch_id_t;
typedef uint8_t comp_size_t;

typedef uint32_t chunk_size_t;
typedef uint32_t chunks_length_t;

typedef struct ComponentData {
    comp_size_t number_of_fields;
    comp_size_t* field_sizes;
} ComponentData;

typedef struct ArchetypeDataChunk {
    id_t* id_dense_array;
    void*** component_field_arrays;
    chunk_size_t dense_arrays_length;
} ArchetypeDataChunk;

typedef struct Archetype {
    comp_id_t* components;
    ArchetypeDataChunk* chunks;
    chunks_length_t number_of_chunks;
    chunk_size_t chunk_size;
    comp_id_t number_of_components;
    arch_id_t archetype_id;
} Archetype;

typedef struct SparseArrayChunk {
    arch_id_t* archetypes;
    chunks_length_t* chunk_indexes;
    id_t* dense_id_array_indexes;
} SparseArrayChunk;

typedef struct World {
    Archetype* archetypes;
    id_t* id_stack_ids;
    id_t id_stack_capacity;
    id_t id_stack_top_index;
    comp_id_t* component_ids;
    ComponentData* all_components_data;

    SparseArrayChunk* sparse_array_chunks;
    const chunk_size_t sparse_array_chunk_size;
    chunks_length_t starting_sparse_array_chunks;
    chunks_length_t sparse_array_number_of_chunks;

    const chunk_size_t dense_array_chunk_size;
    arch_id_t number_of_archetypes;
    comp_id_t number_of_components;
} World;

World world_create(
    const chunk_size_t dense_array_chunk_size,
    const chunk_size_t sparse_array_chunk_size,
    const chunks_length_t starting_sparse_array_chunks
);

void world_destroy(World* world);

comp_id_t world_add_component_type(World* world, const comp_size_t* field_sizes, comp_size_t number_of_fields);

id_t world_add_entity(World* world, comp_id_t number_of_components, const comp_id_t* components);

void world_remove_entity(World* world, const id_t entity_id);

void* world_get_component_field(
    const World* world,
    const id_t entity_id,
    const comp_id_t component_id,
    const comp_size_t field_index
);

typedef struct ComponentIterator {
    void**** component_field_arrays;
    chunk_size_t* chunk_lengths;
    chunks_length_t number_of_chunks;
} ComponentIterator;

ComponentIterator world_get_component_iterator(const World* world, const comp_id_t* component_ids, const comp_id_t number_of_components);

void component_iterator_destroy(ComponentIterator* iterator);

#endif //ECS_H