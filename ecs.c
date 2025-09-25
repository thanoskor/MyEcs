#include "ecs.h"

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


// Comparison function for qsort from stdlib
static int compare_comp_ids(const void* a, const void* b) {
    comp_id_t id_a = *(const comp_id_t*)a;
    comp_id_t id_b = *(const comp_id_t*)b;
    if (id_a < id_b) return -1;
    if (id_a > id_b) return 1;
    return 0;
}


//ComponentData Functions
void component_data_init(
    ComponentData* this,
    const comp_size_t* field_sizes,
    const comp_size_t number_of_fields) {

    this->number_of_fields = number_of_fields;
    this->field_sizes = malloc(sizeof(comp_size_t) * number_of_fields);
    if(!this->field_sizes) exit(EXIT_FAILURE);
    for (comp_size_t i=0; i<number_of_fields; i++) {
        this->field_sizes[i] = field_sizes[i];
    }
}

void component_data_destroy(ComponentData* this) {
    free(this->field_sizes);
    this->field_sizes = NULL;
}


//ArchetypeDataChunk Functions
void archetype_data_chunk_init(
    ArchetypeDataChunk* this,
    const ComponentData* all_components_data,
    const comp_id_t number_of_all_components,
    const comp_id_t number_of_archetype_components,
    const comp_id_t* component_ids_of_archetype,
    const size_t chunk_size) {

    this->dense_arrays_length = 0;

    this->id_dense_array = _aligned_malloc(sizeof(id_t) * chunk_size, CACHE_SIZE);
    if(!this->id_dense_array) exit(EXIT_FAILURE);

    this->component_field_arrays = malloc(sizeof(void**) * number_of_all_components);
    if(!this->component_field_arrays) exit(EXIT_FAILURE);

    //initialize all to NULL
    for (comp_id_t i = 0; i < number_of_all_components; i++) {
        this->component_field_arrays[i] = NULL;
    }

    //only instantiate id indexes of active components
    for (comp_id_t i = 0; i < number_of_archetype_components; i++) {
        const comp_id_t current_comp_id = component_ids_of_archetype[i];
        comp_size_t number_of_fields = all_components_data[current_comp_id].number_of_fields;
        comp_size_t* field_sizes = all_components_data[current_comp_id].field_sizes;

        this->component_field_arrays[current_comp_id] = malloc(sizeof(void*) * number_of_fields);
        if(!this->component_field_arrays[current_comp_id]) exit(EXIT_FAILURE);

        for (comp_size_t j=0; j<number_of_fields; j++) {
            this->component_field_arrays[current_comp_id][j] = _aligned_malloc(field_sizes[j] * chunk_size, CACHE_SIZE);
            if(!this->component_field_arrays[current_comp_id][j]) exit(EXIT_FAILURE);
        }
    }
}

void archetype_data_chunk_destroy(ArchetypeDataChunk* chunk, const ComponentData* all_components_data, comp_id_t number_of_all_components) {
    _aligned_free(chunk->id_dense_array);

    for (comp_id_t i = 0; i < number_of_all_components; i++) {
        if (chunk->component_field_arrays[i] != NULL) {
            comp_size_t number_of_fields = all_components_data[i].number_of_fields;
            for (comp_size_t j = 0; j < number_of_fields; j++) {
                _aligned_free(chunk->component_field_arrays[i][j]);
            }
            free(chunk->component_field_arrays[i]);
        }
    }
    free(chunk->component_field_arrays);
}


//Archetype Functions
void archetype_init(
    Archetype* this,
    const arch_id_t archetype_id,
    const comp_id_t number_of_all_components,
    const ComponentData* all_components_data,
    const comp_id_t number_of_archetype_components,
    const comp_id_t* component_ids_of_archetype,
    const size_t chunk_size,
    const size_t number_of_chunks) {

    this->number_of_chunks = number_of_chunks;
    this->chunk_size = chunk_size;
    this->number_of_components = number_of_archetype_components;
    this->archetype_id = archetype_id;

    this->chunks = malloc(sizeof(ArchetypeDataChunk) * number_of_chunks);
    if(!this->chunks) exit(EXIT_FAILURE);

    for (size_t i = 0; i < number_of_chunks; i++) {
        archetype_data_chunk_init(
            &this->chunks[i],
            all_components_data,
            number_of_all_components,
            number_of_archetype_components,
            component_ids_of_archetype,
            chunk_size);
    }

    this->components = malloc(sizeof(comp_id_t) * number_of_archetype_components);
    if(!this->components) exit(EXIT_FAILURE);

    for (int i = 0; i < number_of_archetype_components; i++) {
        this->components[i] = component_ids_of_archetype[i];
    }
}

void archetype_destroy(Archetype* archetype, const ComponentData* all_components_data, comp_id_t number_of_all_components) {
    for (chunks_length_t i = 0; i < archetype->number_of_chunks; i++) {
        archetype_data_chunk_destroy(&archetype->chunks[i], all_components_data, number_of_all_components);
    }
    free(archetype->chunks);
    free(archetype->components);
}

void archetype_add_entity(
    Archetype* archetype,
    const id_t entity_id,
    const comp_id_t number_of_all_components,
    const ComponentData* all_components_data,
    id_t* id_dense_array_index,
    chunk_size_t* chunk_index)
{
    for (size_t i = 0; i < archetype->number_of_chunks; i++) {
        if (archetype->chunks[i].dense_arrays_length < archetype->chunk_size) {
            archetype->chunks[i].id_dense_array[archetype->chunks[i].dense_arrays_length] = entity_id;
            *id_dense_array_index = archetype->chunks[i].dense_arrays_length++;
            *chunk_index = i;
            return;
        }
    }
    //if no space inside any chunk we allocate a new and recall the function
    archetype->number_of_chunks++;
    archetype->chunks = realloc(archetype->chunks, sizeof(ArchetypeDataChunk) * archetype->number_of_chunks);
    if(!archetype->chunks) exit(EXIT_FAILURE);

    archetype_data_chunk_init(
            &archetype->chunks[archetype->number_of_chunks - 1],
            all_components_data,
            number_of_all_components,
            archetype->number_of_components,
            archetype->components,
            archetype->chunk_size);

    archetype->chunks[archetype->number_of_chunks - 1].id_dense_array[0] = entity_id;
    archetype->chunks[archetype->number_of_chunks - 1].dense_arrays_length++;
    *id_dense_array_index = 0;
    *chunk_index = archetype->number_of_chunks - 1;
}


//SparseArrayChunk Functions
void sparse_array_chunk_init(SparseArrayChunk* this, chunk_size_t size) {
    this->archetypes = malloc(sizeof(arch_id_t) * size);
    if(!this->archetypes) exit(EXIT_FAILURE);
    this->chunk_indexes = malloc(sizeof(chunks_length_t) * size);
    if(!this->chunk_indexes) exit(EXIT_FAILURE);
    this->dense_id_array_indexes = malloc(sizeof(id_t) * size);
    if(!this->dense_id_array_indexes) exit(EXIT_FAILURE);
}

void sparse_array_chunk_destroy(SparseArrayChunk* this) {
    free(this->archetypes);
    free(this->chunk_indexes);
    free(this->dense_id_array_indexes);
}


//World Functions
World world_create(
    const chunk_size_t dense_array_chunk_size,
    const chunk_size_t sparse_array_chunk_size,
    const chunks_length_t starting_sparse_array_chunks
) {
    World this = {
        .archetypes = NULL,
        .id_stack_ids = NULL,
        .id_stack_capacity = sparse_array_chunk_size,
        .id_stack_top_index = 0,
        .component_ids = NULL,
        .all_components_data = NULL,
        .sparse_array_chunks = NULL,
        .sparse_array_chunk_size = sparse_array_chunk_size,
        .sparse_array_number_of_chunks = starting_sparse_array_chunks,
        .dense_array_chunk_size = dense_array_chunk_size,
        .number_of_archetypes = 0,
        .number_of_components = 0
    };

    this.id_stack_ids = malloc(sizeof(id_t) * this.id_stack_capacity);
    if(!this.id_stack_ids) exit(EXIT_FAILURE);
    for (id_t i = 0; i < this.id_stack_capacity; i++) {
        this.id_stack_ids[i] = i;
    }

    this.sparse_array_chunks = malloc(sizeof(SparseArrayChunk) * starting_sparse_array_chunks);
    if(!this.sparse_array_chunks) exit(EXIT_FAILURE);

    for (chunks_length_t c=0; c< starting_sparse_array_chunks; c++) {
        sparse_array_chunk_init(&this.sparse_array_chunks[c], this.sparse_array_chunk_size);
    }

    return this;
}

void world_destroy(World* world) {
    for (arch_id_t i = 0; i < world->number_of_archetypes; i++) {
        archetype_destroy(&world->archetypes[i], world->all_components_data, world->number_of_components);
    }
    free(world->archetypes);

    for (comp_id_t i = 0; i < world->number_of_components; i++) {
        component_data_destroy(&world->all_components_data[i]);
    }
    free(world->all_components_data);
    free(world->component_ids);

    for (chunks_length_t i = 0; i < world->sparse_array_number_of_chunks; i++) {
        sparse_array_chunk_destroy(&world->sparse_array_chunks[i]);
    }
    free(world->sparse_array_chunks);

    free(world->id_stack_ids);
}

static bool world_match_archetype(const World* world, comp_id_t number_of_components, const comp_id_t* components, arch_id_t* matched_arch_id) {
    for (arch_id_t a = 0; a < world->number_of_archetypes; a++) {
        if (world->archetypes[a].number_of_components == number_of_components) {
            bool matched = true;
            for (comp_id_t c = 0; c < number_of_components; c++) {
                if (world->archetypes[a].components[c] != components[c]) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                *matched_arch_id = a;
                return true;
            }
        }
    }
    return false;
}

static arch_id_t world_add_archetype(World* world, const comp_id_t number_of_components, const comp_id_t* components) {
    world->archetypes = realloc(world->archetypes, (world->number_of_archetypes + 1) * sizeof(Archetype));
    if(!world->archetypes) exit(EXIT_FAILURE);

    size_t number_of_chunks = 1;

    archetype_init(
        &world->archetypes[world->number_of_archetypes],
        world->number_of_archetypes,
        world->number_of_components,
        world->all_components_data,
        number_of_components,
        components,
        world->dense_array_chunk_size,
        number_of_chunks);

    return world->number_of_archetypes++;
}

comp_id_t world_add_component_type(World* world, const comp_size_t* field_sizes, const comp_size_t number_of_fields) {
    world->component_ids = realloc(
        world->component_ids,
        sizeof(comp_id_t) * (world->number_of_components + 1));
    if(!world->component_ids) exit(EXIT_FAILURE);

    world->all_components_data = realloc(
        world->all_components_data,
        sizeof(ComponentData) * (world->number_of_components + 1));
    if(!world->all_components_data) exit(EXIT_FAILURE);

    component_data_init(
        &world->all_components_data[world->number_of_components],
        field_sizes,
        number_of_fields);

    return world->number_of_components++;
}

id_t world_add_entity(World* world, const comp_id_t number_of_components, const comp_id_t* components) {
    //pop an id from the stack
    if (world->id_stack_top_index >= world->id_stack_capacity) {
        world->id_stack_capacity *= 2;
        world->id_stack_ids = realloc(world->id_stack_ids, sizeof(id_t) * world->id_stack_capacity);
        if(!world->id_stack_ids) exit(EXIT_FAILURE);
        // Initialize new part of the stack
        for(id_t i = world->id_stack_top_index; i < world->id_stack_capacity; ++i) {
            world->id_stack_ids[i] = i;
        }
    }
    const id_t id = world->id_stack_ids[world->id_stack_top_index++];

    //sort components so that order does not matter
    comp_id_t* sorted_components = alloca(sizeof(comp_id_t) * number_of_components);
    if(!sorted_components) exit(EXIT_FAILURE);
    for(comp_id_t i = 0; i < number_of_components; ++i) {
        sorted_components[i] = components[i];
    }
    qsort(sorted_components, number_of_components, sizeof(comp_id_t), compare_comp_ids);


    chunks_length_t chunk_index = 0;
    id_t id_dense_array_index = 0;
    arch_id_t matched_arch_id = 0;

    //try to fit to already existing archetype
    if (world_match_archetype(world, number_of_components, sorted_components, &matched_arch_id)) {
        archetype_add_entity(
            &world->archetypes[matched_arch_id],
            id,
            world->number_of_components,
            world->all_components_data,
            &id_dense_array_index,
            &chunk_index);
    }
    //if no such archetype exists create a new archetype
    else {
        matched_arch_id = world_add_archetype(world, number_of_components, sorted_components);
        archetype_add_entity(
            &world->archetypes[matched_arch_id],
            id,
            world->number_of_components,
            world->all_components_data,
            &id_dense_array_index,
            &chunk_index);
    }

    const chunks_length_t sparse_chunk_index = id / world->sparse_array_chunk_size;

    if (sparse_chunk_index >= world->sparse_array_number_of_chunks) {
        chunks_length_t new_chunk_count = world->sparse_array_number_of_chunks + 1;
        world->sparse_array_chunks = realloc(
            world->sparse_array_chunks,
            sizeof(SparseArrayChunk) * new_chunk_count
        );
        if(!world->sparse_array_chunks) exit(EXIT_FAILURE);

        for(chunks_length_t i = world->sparse_array_number_of_chunks; i < new_chunk_count; ++i) {
            sparse_array_chunk_init(
                &world->sparse_array_chunks[i],
                world->sparse_array_chunk_size);
        }
        world->sparse_array_number_of_chunks = new_chunk_count;
    }

    const id_t local_id_index = id % world->sparse_array_chunk_size;

    world->sparse_array_chunks[sparse_chunk_index].archetypes[local_id_index] = matched_arch_id;
    world->sparse_array_chunks[sparse_chunk_index].chunk_indexes[local_id_index] = chunk_index;
    world->sparse_array_chunks[sparse_chunk_index].dense_id_array_indexes[local_id_index] = id_dense_array_index;
    return id;
}

static bool does_archetype_have_components(const Archetype* archetype, const comp_id_t* comp_ids, const comp_id_t number_of_comps) {
    if (archetype->number_of_components < number_of_comps) {
        return false;
    }
    for (comp_id_t c = 0; c < number_of_comps; ++c) {
        bool found_match = false;
        for (comp_id_t ac = 0; ac < archetype->number_of_components; ++ac) {
            if (archetype->components[ac] == comp_ids[c]) {
                found_match = true;
                break;
            }
        }
        if (!found_match) {
            return false;
        }
    }
    return true;
}

void world_remove_entity(World* world, const id_t entity_id) {

    //return the deleted ID to the stack
    world->id_stack_ids[--world->id_stack_top_index] = entity_id;

    //find the location of the entity to be removed
    const chunks_length_t sparse_array_chunk_index = entity_id / world->sparse_array_chunk_size;
    assert(sparse_array_chunk_index < world->sparse_array_number_of_chunks);

    const SparseArrayChunk* sparse_array_chunk = &world->sparse_array_chunks[sparse_array_chunk_index];
    const id_t local_id_index = entity_id % world->sparse_array_chunk_size;

    const arch_id_t archetype_id = sparse_array_chunk->archetypes[local_id_index];
    const chunks_length_t chunk_index = sparse_array_chunk->chunk_indexes[local_id_index];
    const id_t dense_id_array_index = sparse_array_chunk->dense_id_array_indexes[local_id_index];

    Archetype* archetype = &world->archetypes[archetype_id];
    ArchetypeDataChunk* archetype_data_chunk = &archetype->chunks[chunk_index];

    const id_t dense_id_array_length = archetype_data_chunk->dense_arrays_length;
    const id_t last_element_index = dense_id_array_length - 1;

    // If the entity to be removed is the last one in the array,
    // we can simply decrement the length. No swapping is needed.
    if (dense_id_array_index == last_element_index) {
        archetype_data_chunk->dense_arrays_length--;
        return;
    }

    //get the ID of the entity at the last position.
    const id_t last_entity_id = archetype_data_chunk->id_dense_array[last_element_index];

    //move the last entity's ID into the deleted entity's slot.
    archetype_data_chunk->id_dense_array[dense_id_array_index] = last_entity_id;

    //move the last entity's component data into the deleted entity's slot.
    for (comp_id_t c=0; c<archetype->number_of_components; c++) {
        const comp_id_t component_index = archetype->components[c];
        const ComponentData* component_data = &world->all_components_data[component_index];
        for (comp_size_t f=0; f<component_data->number_of_fields; f++) {
            comp_size_t field_size = component_data->field_sizes[f];
            void* dest = archetype_data_chunk->component_field_arrays[component_index][f] + (dense_id_array_index * field_size);
            const void* src = archetype_data_chunk->component_field_arrays[component_index][f] + (last_element_index * field_size);
            memcpy(dest, src, field_size);
        }
    }


    //update the sparse array for the moved entity
    const chunks_length_t sparse_chunk_for_moved_entity = last_entity_id / world->sparse_array_chunk_size;
    const id_t local_id_for_moved_entity = last_entity_id % world->sparse_array_chunk_size;
    world->sparse_array_chunks[sparse_chunk_for_moved_entity].dense_id_array_indexes[local_id_for_moved_entity] = dense_id_array_index;

    archetype_data_chunk->dense_arrays_length--;
}

void* world_get_component_field(
    const World* world,
    const id_t entity_id,
    const comp_id_t component_id,
    const comp_size_t field_index
)
{
    //find the entity's location using the sparse array
    const chunks_length_t sparse_chunk_index = entity_id / world->sparse_array_chunk_size;
    if (sparse_chunk_index >= world->sparse_array_number_of_chunks) {
        return NULL; // Entity ID is out of bounds
    }

    const SparseArrayChunk* sparse_array_chunk = &world->sparse_array_chunks[sparse_chunk_index];
    const id_t local_id_index = entity_id % world->sparse_array_chunk_size;

    const arch_id_t archetype_id = sparse_array_chunk->archetypes[local_id_index];
    const chunks_length_t chunk_index = sparse_array_chunk->chunk_indexes[local_id_index];
    const id_t dense_id_array_index = sparse_array_chunk->dense_id_array_indexes[local_id_index];

    const Archetype* archetype = &world->archetypes[archetype_id];

    //validate that the entity actually has this component
    bool has_component = false;
    for (comp_id_t i = 0; i < archetype->number_of_components; ++i) {
        if (archetype->components[i] == component_id) {
            has_component = true;
            break;
        }
    }
    if (!has_component) {
        return NULL; //this entity does not have the requested component
    }

    //validate the field index
    const ComponentData* component_data = &world->all_components_data[component_id];
    if (field_index >= component_data->number_of_fields) {
        return NULL; // The component does not have this many fields
    }

    //calculate the pointer to the specific field data
    const ArchetypeDataChunk* archetype_data_chunk = &archetype->chunks[chunk_index];
    const comp_size_t field_size = component_data->field_sizes[field_index];

    //get the base address of the dense array for this specific component field
    uint8_t* field_array_base = archetype_data_chunk->component_field_arrays[component_id][field_index];

    //calculate the offset to the entity's data within that array
    return field_array_base + (dense_id_array_index * field_size);
}

//ComponentIterator Functions
ComponentIterator world_get_component_iterator(const World* world, const comp_id_t* component_ids, const comp_id_t number_of_components) {
    ComponentIterator iterator = { .component_field_arrays = NULL, .chunk_lengths = NULL, .number_of_chunks = 0 };
    chunks_length_t total_chunks = 0;

    for (arch_id_t a = 0; a < world->number_of_archetypes; a++) {
        if (does_archetype_have_components(&world->archetypes[a], component_ids, number_of_components)) {
            total_chunks += world->archetypes[a].number_of_chunks;
        }
    }

    if (total_chunks == 0) {
        return iterator;
    }

    iterator.component_field_arrays = malloc(sizeof(void***) * total_chunks);
    if(!iterator.component_field_arrays) exit(EXIT_FAILURE);

    iterator.chunk_lengths = malloc(sizeof(chunk_size_t) * total_chunks);
    if(!iterator.chunk_lengths) exit(EXIT_FAILURE);

    iterator.number_of_chunks = total_chunks;

    chunks_length_t current_chunk_index = 0;
    for (arch_id_t a = 0; a < world->number_of_archetypes; a++) {
        Archetype* archetype = &world->archetypes[a];

        if (does_archetype_have_components(archetype, component_ids, number_of_components)) {
            for (chunks_length_t ch = 0; ch < archetype->number_of_chunks; ch++) {
                iterator.component_field_arrays[current_chunk_index] = malloc(sizeof(void**) * number_of_components);
                if(!iterator.component_field_arrays[current_chunk_index]) exit(EXIT_FAILURE);

                for (comp_id_t co = 0; co < number_of_components; co++) {
                    iterator.component_field_arrays[current_chunk_index][co] =
                        archetype->chunks[ch].component_field_arrays[component_ids[co]];
                }
                iterator.chunk_lengths[current_chunk_index] = archetype->chunks[ch].dense_arrays_length;
                current_chunk_index++;
            }
        }
    }
    return iterator;
}

void component_iterator_destroy(ComponentIterator* iterator) {
    if (iterator->component_field_arrays) {
        for (chunks_length_t i = 0; i < iterator->number_of_chunks; i++) {
            free(iterator->component_field_arrays[i]); // Free the inner array
        }
        free(iterator->component_field_arrays); // Free the outer array
        iterator->component_field_arrays = NULL;
    }
    if (iterator->chunk_lengths) {
        free(iterator->chunk_lengths);
        iterator->chunk_lengths = NULL;
    }
    iterator->number_of_chunks = 0;
}