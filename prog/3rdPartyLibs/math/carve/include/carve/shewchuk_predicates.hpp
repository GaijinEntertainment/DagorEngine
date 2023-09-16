
namespace shewchuk {
double orient2dfast(const double* pa, const double* pb, const double* pc);
double orient2dexact(const double* pa, const double* pb, const double* pc);
double orient2dslow(const double* pa, const double* pb, const double* pc);
double orient2dadapt(const double* pa, const double* pb, const double* pc,
                     double detsum);
double orient2d(const double* pa, const double* pb, const double* pc);

double orient3dfast(const double* pa, const double* pb, const double* pc,
                    const double* pd);
double orient3dexact(const double* pa, const double* pb, const double* pc,
                     const double* pd);
double orient3dslow(const double* pa, const double* pb, const double* pc,
                    const double* pd);
double orient3dadapt(const double* pa, const double* pb, const double* pc,
                     const double* pd, double permanent);
double orient3d(const double* pa, const double* pb, const double* pc,
                const double* pd);

double incirclefast(const double* pa, const double* pb, const double* pc,
                    const double* pd);
double incircleexact(const double* pa, const double* pb, const double* pc,
                     const double* pd);
double incircleslow(const double* pa, const double* pb, const double* pc,
                    const double* pd);
double incircleadapt(const double* pa, const double* pb, const double* pc,
                     const double* pd, double permanent);
double incircle(const double* pa, const double* pb, const double* pc,
                const double* pd);

double inspherefast(const double* pa, const double* pb, const double* pc,
                    const double* pd, const double* pe);
double insphereexact(const double* pa, const double* pb, const double* pc,
                     const double* pd, const double* pe);
double insphereslow(const double* pa, const double* pb, const double* pc,
                    const double* pd, const double* pe);
double insphereadapt(const double* pa, const double* pb, const double* pc,
                     const double* pd, const double* pe, double permanent);
double insphere(const double* pa, const double* pb, const double* pc,
                const double* pd, const double* pe);
}  // namespace shewchuk
