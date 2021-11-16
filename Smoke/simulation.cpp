#include "simulation.h"

#include <QDebug>

#include <cmath>

//init_simulation: Initialize simulation data structures as a function of the grid size 'n'. (This used to be init_simulation.)
//                 Although the simulation takes place on a 2D grid, we allocate all data structures as 1D arrays,
//                 for compatibility with the FFTW numerical library.
Simulation::Simulation(size_t const DIM)
    :
      m_DIM(DIM)
{
    initializeDataStructures();
}

void Simulation::initializeDimensions(size_t const DIM)
{
    m_DIM = DIM;
    m_numberOfSamples = m_DIM * m_DIM;
    m_numberOfSamplesLong = static_cast<long>(m_numberOfSamples);
}

void Simulation::initializeDataStructures()
{
    m_fx.resize(  m_numberOfSamples, 0.0F);
    m_fy.resize(  m_numberOfSamples, 0.0F);
    m_rho.resize( m_numberOfSamples, 0.0F);
    m_rho0.resize(m_numberOfSamples, 0.0F);

    size_t const numberOfVelocitySamples = m_DIM * m_DIM + (2 * m_DIM);

    m_vx.resize( numberOfVelocitySamples, 0.0F);
    m_vy.resize( numberOfVelocitySamples, 0.0F);
    m_vx0.resize(numberOfVelocitySamples, 0.0F);
    m_vy0.resize(numberOfVelocitySamples, 0.0F);

    // Forward plans.
    int const m_DIM_int = static_cast<int>(m_DIM);
    m_plan_realToComplexVx = fftwf_plan_dft_r2c_2d(m_DIM_int,
                                                   m_DIM_int,
                                                   m_vx0.data(),
                                                   reinterpret_cast<fftwf_complex*>(m_vx0.data()),
                                                   FFTW_MEASURE);
    m_plan_realToComplexVy = fftwf_plan_dft_r2c_2d(m_DIM_int,
                                                   m_DIM_int,
                                                   m_vy0.data(),
                                                   reinterpret_cast<fftwf_complex*>(m_vy0.data()),
                                                   FFTW_MEASURE);

    // Backward plans.
    m_plan_complexToRealVx = fftwf_plan_dft_c2r_2d(m_DIM_int,
                                                   m_DIM_int,
                                                   reinterpret_cast<fftwf_complex*>(m_vx0.data()),
                                                   m_vx0.data(),
                                                   FFTW_MEASURE);
    m_plan_complexToRealVy = fftwf_plan_dft_c2r_2d(m_DIM_int,
                                                   m_DIM_int,
                                                   reinterpret_cast<fftwf_complex*>(m_vy0.data()),
                                                   m_vy0.data(),
                                                   FFTW_MEASURE);
}

void Simulation::resetData()
{
    std::fill(m_fx.begin(), m_fx.end(), 0.0F);
    std::fill(m_fy.begin(), m_fy.end(), 0.0F);
    std::fill(m_rho.begin(), m_rho.end(), 0.0F);
    std::fill(m_rho0.begin(), m_rho0.end(), 0.0F);

    std::fill(m_vx.begin(), m_vx.end(), 0.0F);
    std::fill(m_vy.begin(), m_vy.end(), 0.0F);
    std::fill(m_vx0.begin(), m_vx0.end(), 0.0F);
    std::fill(m_vy0.begin(), m_vy0.end(), 0.0F);
}

Simulation::~Simulation()
{
    destructFftw();
}

void Simulation::destructFftw()
{
    fftwf_destroy_plan(m_plan_realToComplexVx);
    fftwf_destroy_plan(m_plan_realToComplexVy);
    fftwf_destroy_plan(m_plan_complexToRealVx);
    fftwf_destroy_plan(m_plan_complexToRealVy);

    fftwf_cleanup();
}

void Simulation::solve()
{
    // n is an integer alias for m_DIM.
    int const n = static_cast<int>(m_DIM);

    // Use the underlying pointer to access the elements using integers/longs instead of size_t and casts.
    float * const vx  = m_vx.data();
    float * const vy  = m_vy.data();
    float * const vx0 = m_vx0.data();
    float * const vy0 = m_vy0.data();

    // TODO: Create better scoping for these variables.
    float x, y, x0, y0, f, r, U[2], V[2], s, t;
    int i, j, i0, j0, i1, j1; // TODO: Making these size_t's somehow doesn't work?

    auto const applyTimeStep = [=](float const v, float const v0) { return v + m_dt * v0; };
    std::transform(vx, vx + m_numberOfSamples, vx0, vx, applyTimeStep);
    std::transform(vy, vy + m_numberOfSamples, vy0, vy, applyTimeStep);

    std::copy_n(vx, m_numberOfSamples, vx0);
    std::copy_n(vy, m_numberOfSamples, vy0);

    for (i = 0, x = 0.5F / n; i < n; ++i, x += 1.0F / n)
        for (j = 0, y = 0.5F / n; j < n; ++j, y += 1.0F / n)
        {
            x0 = n * (x - m_dt * vx0[i + n * j]) - 0.5F;
            y0 = n * (y - m_dt * vy0[i + n * j]) - 0.5F;
            i0 = static_cast<int>(std::floor(x0));
            s = x0 - i0;
            i0 = (n + (i0 % n)) % n;
            i1 = (i0 + 1) % n;
            j0 = static_cast<int>(std::floor(y0));
            t = y0 - j0;
            j0 = (n + (j0 % n)) % n;
            j1 = (j0 + 1) % n;

            vx[i + n * j] = (1 - s) * ((1 - t) * vx0[i0 + n * j0]
                          + t * vx0[i0 + n * j1])
                          + s * ((1 - t) * vx0[i1 + n * j0]
                          + t * vx0[i1 + n * j1]);

            vy[i + n * j] = (1 - s) * ((1 - t) * vy0[i0 + n * j0]
                          + t * vy0[i0 + n * j1])
                          + s * ((1 - t) * vy0[i1 + n * j0]
                          + t * vy0[i1 + n * j1]);
        }

    for(size_t j = 0; j < m_DIM; ++j)
        for(size_t i = 0; i < m_DIM; ++i)
        {
            size_t const writeIdx = i + (m_DIM + 2) * j;
            size_t const readIdx = i + m_DIM * j;
            m_vx0[writeIdx] = m_vx[readIdx];
            m_vy0[writeIdx] = m_vy[readIdx];
        }

    // Forward FFT
    fftwf_execute(m_plan_realToComplexVx);
    fftwf_execute(m_plan_realToComplexVy);

    for (i = 0; i <= n; i+= 2)
    {
        x = 0.5F * i;
        for (j = 0; j < n; ++j)
        {
            y =  j <= (n / 2) ? static_cast<float>(j) : static_cast<float>(j) - n;
            r = (x * x) + (y * y);
            if (r == 0.0F)
                continue;

            f = std::exp(-r * m_dt * m_viscosity);
            U[0] = vx0[i     + (n + 2) * j];
            V[0] = vy0[i     + (n + 2) * j];

            U[1] = vx0[i + 1 + (n + 2) * j];
            V[1] = vy0[i + 1 + (n + 2) * j];

            vx0[i     + (n + 2) * j] = f * ((1 - x * x / r) * U[0] - x * y / r * V[0]);
            vx0[i + 1 + (n + 2) * j] = f * ((1 - x * x / r) * U[1] - x * y / r * V[1]);

            vy0[i     + (n + 2) * j] = f * (-y * x / r * U[0] + (1 - y * y / r) * V[0]);
            vy0[i + 1 + (n + 2) * j] = f * (-y * x / r * U[1] + (1 - y * y / r) * V[1]);
       }
    }

    // Backward FFT
    fftwf_execute(m_plan_complexToRealVx);
    fftwf_execute(m_plan_complexToRealVy);

    f = 1.0F / (n * n);
    for (size_t j = 0; j < m_DIM; ++j)
        for (size_t i = 0; i < m_DIM; ++i)
        {
            vx[i + m_DIM * j] = f * vx0[i + (m_DIM + 2) * j];
            vy[i + m_DIM * j] = f * vy0[i + (m_DIM + 2) * j];
        }
}

// diffuse_matter: This function diffuses matter that has been placed in the velocity field. It's almost identical to the
// velocity diffusion step in the function above. The input matter densities are in m_rho0 and the result is written into m_rho.
// TODO: n == m_DIM and should be a size_t.
void Simulation::diffuse_matter()
{
    // n is an integer alias for m_DIM.
    int const n = static_cast<int>(m_DIM);

    // Use the underlying pointer to access the elements using integers/longs instead of size_t and casts.
    float * const vx  = m_vx.data();
    float * const vy  = m_vy.data();
    float * const rho = m_rho.data();
    float * const rho0 = m_rho0.data();

    // TODO: Create better scopes for these variables.
    float x, y, x0, y0, s, t;
    int i, j, i0, j0, i1, j1;

    for (j = 0, y = 0.5F / n; j < n; ++j, y += 1.0F / n)
        for (i = 0, x = 0.5F / n; i < n ; ++i, x += 1.0F / n)
        {
            x0 = n * (x - m_dt * vx[i + n * j]) - 0.5F;
            y0 = n * (y - m_dt * vy[i + n * j]) - 0.5F;
            i0 = static_cast<int>(std::floor(x0));
            s = x0 - i0;
            i0 = (n + (i0 % n)) % n;
            i1 = (i0 + 1) % n;
            j0 = static_cast<int>(std::floor(y0));
            t = y0 - j0;
            j0 = (n + (j0 % n)) % n;
            j1 = (j0 + 1) % n;
            rho[i + n * j] = (1 - s) * ((1 - t) * rho0[i0 + n * j0]
                                    + t * rho0[i0 + n * j1])
                                    + s * ((1 - t) * rho0[i1 + n * j0]
                                    + t * rho0[i1 + n * j1]);
        }
}

//set_forces: copy user-controlled forces to the force vectors that are sent to the solver.
//            Also dampen forces and matter density to get a stable simulation.
void Simulation::set_forces()
{
    // Reduce density and copy to current density.
    std::transform(m_rho.cbegin(), m_rho.cend(), m_rho0.begin(),
                   std::bind(std::multiplies<>(), std::placeholders::_1, 0.995F));

    // Reduce force.
    std::transform(m_fx.begin(), m_fx.end(), m_fx.begin(),
                   std::bind(std::multiplies<>(), std::placeholders::_1, 0.85F));
    std::transform(m_fy.begin(), m_fy.end(), m_fy.begin(),
                   std::bind(std::multiplies<>(), std::placeholders::_1, 0.85F));

    // Copy forces to velocities.
    std::copy(m_fx.cbegin(), m_fx.cend(), m_vx0.begin());
    std::copy(m_fy.cbegin(), m_fy.cend(), m_vy0.begin());
}

//do_one_simulation_step: Do one complete cycle of the simulation:
//      - set_forces:
//      - solve:            read forces from the user
//      - diffuse_matter:   compute a new set of velocities
//      - gluPostRedisplay: draw a new visualization frame

// TODO: Make this function NOT run when the state is static.
void Simulation::do_one_simulation_step()
{
    set_forces();
    solve();
    diffuse_matter();
}


// Getters

// Unfortunately, copying is necessary to consistently export std::vector<float> vectors,
// without the custom allocator.
std::vector<float> Simulation::density() const
{
    return std::vector<float>{m_rho.cbegin(), m_rho.cend()};
}

// Note that the dimensions of m_vx and m_vy are larger than what is returned.
// This is because the internal algorithm needs one more row and column.
std::vector<float> Simulation::velocityMagnitude() const
{
    auto const length = [](auto const vx, auto const vy) { return std::sqrt(std::pow(vx, 2.0F) + std::pow(vy, 2.0F)); };

    std::vector<float> velocityMagnitude;
    std::transform(m_vx.cbegin(), m_vx.cbegin() + m_numberOfSamplesLong, m_vy.cbegin(), std::back_inserter(velocityMagnitude),
                   length);

    return velocityMagnitude;
}

std::vector<float> Simulation::forceFieldMagnitude() const
{
    auto const length = [](auto const fx, auto const fy) { return std::sqrt(std::pow(fx, 2.0F) + std::pow(fy, 2.0F)); };

    std::vector<float> forceFieldMagnitude;
    std::transform(m_fx.cbegin(), m_fx.cend(), m_fy.cbegin(), std::back_inserter(forceFieldMagnitude),
                   length);

   return forceFieldMagnitude;
}

float Simulation::dt() const
{
    return m_dt;
}

float Simulation::viscosity() const
{
    return m_viscosity;
}

float Simulation::rhoInjected() const
{
    return m_rhoInjected;
}

float Simulation::vx(size_t const idx) const
{
    return m_vx[idx];
}

float Simulation::vy(size_t const idx) const
{
    return m_vy[idx];
}

float Simulation::vx0(size_t const idx) const
{
    return m_vx0[idx];
}

float Simulation::vy0(size_t const idx) const
{
    return m_vy0[idx];
}

float Simulation::fx(size_t const idx) const
{
    return m_fx[idx];
}

float Simulation::fy(size_t const idx) const
{
    return m_fy[idx];
}

float Simulation::rho(size_t const idx) const
{
    return m_rho[idx];
}

// Setters
void Simulation::setDIM(size_t const DIM)
{
    initializeDimensions(DIM);
    destructFftw();
    initializeDataStructures();
    resetData(); // TODO: Could be redundant as 0.0F is also passed to resize().
}

void Simulation::setDt(float const dt)
{
    m_dt = dt;
}

void Simulation::setViscosity(float const viscosity)
{
    m_viscosity = viscosity;
}

void Simulation::setRhoInjected(float const rhoInjected)
{
    m_rhoInjected = rhoInjected;
}

void Simulation::setFx(size_t const idx, float const force)
{
    m_fx[idx] = force;
}

void Simulation::setFy(size_t const idx, float const force)
{
    m_fy[idx] = force;
}

void Simulation::setRho(size_t const idx, float const smokeDensity)
{
    m_rho[idx] = smokeDensity;
}
