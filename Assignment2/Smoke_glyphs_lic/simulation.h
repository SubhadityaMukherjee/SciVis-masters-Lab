#ifndef SIMULATION_H
#define SIMULATION_H

#include "fftwf_malloc_allocator.h"

#include <fftw3.h>

#include <memory>
#include <vector>

class Simulation
{
    //--- SIMULATION PARAMETERS ------------------------------------------------------------------------
    size_t m_DIM;                 // Size of simulation grid.
    size_t m_numberOfSamples = m_DIM * m_DIM;
    long m_numberOfSamplesLong = static_cast<long>(m_numberOfSamples);

    float m_dt = 0.4F;                  // Simulation time step.
    float m_viscosity = 0.001F;         // Fluid viscosity.
    float m_rhoInjected = 10.0F;        // The amount of density which is injected.

    typedef std::vector<float, fftwf_malloc_allocator<float>> fftwf_vector;
    fftwf_vector m_vx, m_vy;      // (vx,vy)   = velocity field at the current moment.
    fftwf_vector m_vx0, m_vy0;    // (vx0,vy0) = velocity field at the previous moment.
    fftwf_vector m_fx, m_fy;      // (fx,fy)   = user-controlled simulation forces, steered with the mouse.
    fftwf_vector m_rho, m_rho0;   // Smoke density at the current (rho) and previous (rho0) moment.

    // Simulation domain discretization.
    fftwf_plan m_plan_realToComplexVx, m_plan_realToComplexVy;
    fftwf_plan m_plan_complexToRealVx, m_plan_complexToRealVy;


    // Functions

    // Data management
    void initializeDimensions(size_t const DIM);
    void initializeDataStructures();
    void destructFftw();
    void resetData();

    // Algorithm
    void solve();
    void diffuse_matter();
    void set_forces();

public:
    // Functions
    Simulation(size_t const DIM);
    ~Simulation();

    void do_one_simulation_step();

    // Getters
    std::vector<float> density() const;
    std::vector<float> densityInterpolated(size_t const numberOfRows, size_t const numberOfColumns) const;

    std::vector<float> velocityMagnitude() const;
    std::vector<float> velocityMagnitudeInterpolated(size_t const numberOfRows, size_t const numberOfColums) const;
    std::vector<float> velocityXInterpolated(size_t const numberOfRows, size_t const numberOfColumns) const;
    std::vector<float> velocityYInterpolated(size_t const numberOfRows, size_t const numberOfColumns) const;

    std::vector<float> forceFieldMagnitude() const;
    std::vector<float> forceFieldMagnitudeInterpolated(size_t const numberOfRows, size_t const numberOfColumns) const;
    std::vector<float> forceFieldXInterpolated(size_t const numberOfRows, size_t const numberOfColumns) const;
    std::vector<float> forceFieldYInterpolated(size_t const numberOfRows, size_t const numberOfColumns) const;

    float dt() const;
    float viscosity() const;
    float rhoInjected() const;

    float vx(size_t const idx) const;
    float vy(size_t const idx) const;
    float vx0(size_t const idx) const;
    float vy0(size_t const idx) const;

    float fx(size_t const idx) const;
    float fy(size_t const idx) const;
    float rho(size_t const idx) const;

    // Setters
    void setDIM(size_t const DIM);

    void setDt(float const dt);
    void setViscosity(float const viscosity);
    void setRhoInjected(float const rhoInjected);

    void setFx(size_t const idx, float const force);
    void setFy(size_t const idx, float const force);
    void setRho(size_t const idx, float const smokeDensity);
};

#endif // SIMULATION_H
