#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "timer.h"
#include "ecs.h"

#define ENTITY_COUNT 1000000
volatile double g_sink = 0.0;

int main() {
    // Create world
    World world = world_create(1000000, 1000000, 1);

    // Define components: position (x,y,z) and velocity (x,y,z)
    const comp_size_t pos_fields[] = {sizeof(double), sizeof(double), sizeof(double)};
    comp_id_t pos_id = world_add_component_type(&world, pos_fields, 3);

    const comp_size_t vel_fields[] = {sizeof(double), sizeof(double), sizeof(double)};
    comp_id_t vel_id = world_add_component_type(&world, vel_fields, 3);

    // Create entities with pos+vel
    comp_id_t comps[] = {pos_id, vel_id};
    for (size_t i = 0; i < ENTITY_COUNT; i++) {
        world_add_entity(&world, 2, comps);
    }

    // Get iterator for (pos, vel)
    ComponentIterator it = world_get_component_iterator(&world, comps, 2);

    // Fill with random initial values
    for (chunks_length_t c = 0; c < it.number_of_chunks; c++) {
        double* x = it.component_field_arrays[c][0][0];
        double* y = it.component_field_arrays[c][0][1];
        double* z = it.component_field_arrays[c][0][2];
        double* dx = it.component_field_arrays[c][1][0];
        double* dy = it.component_field_arrays[c][1][1];
        double* dz = it.component_field_arrays[c][1][2];

        for (chunk_size_t i = 0; i < it.chunk_lengths[c]; i++) {
            x[i]  = (double)rand() / RAND_MAX * 100.0;
            y[i]  = (double)rand() / RAND_MAX * 100.0;
            z[i]  = (double)rand() / RAND_MAX * 100.0;
            dx[i] = (double)rand() / RAND_MAX - 0.5;
            dy[i] = (double)rand() / RAND_MAX - 0.5;
            dz[i] = (double)rand() / RAND_MAX - 0.5;
        }
    }

    // --- Benchmark iteration ---
    start_timer("ECS Iteration");
    double sink = 0.0;

    for (chunks_length_t c = 0; c < it.number_of_chunks; c++) {
        double* x = it.component_field_arrays[c][0][0];
        double* y = it.component_field_arrays[c][0][1];
        double* z = it.component_field_arrays[c][0][2];
        double* dx = it.component_field_arrays[c][1][0];
        double* dy = it.component_field_arrays[c][1][1];
        double* dz = it.component_field_arrays[c][1][2];

        for (chunk_size_t i = 0; i < it.chunk_lengths[c]; i++) {
            // Just do some math to simulate work
            x[i] += dx[i];
            y[i] += dy[i];
            z[i] += dz[i];
            sink += sqrt(x[i]*x[i] + y[i]*y[i] + z[i]*z[i]);
        }
    }

    stop_timer();
    component_iterator_destroy(&it);

    // Prevent optimization
    g_sink = sink;
    printf("Sink: %f\n", g_sink);

    // Cleanup
    world_destroy(&world);
    return 0;
}
