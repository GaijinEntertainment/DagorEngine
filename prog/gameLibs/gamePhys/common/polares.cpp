#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix4.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlockUtils.h>

#include <gamePhys/common/polares.h>

using namespace gamephys;

/** Get Cx and Cy from angle of attack (aoa) */
Point2 gamephys::calc_c(const Polares &polares, float aoa, float ang, float cl_add, float cd_coeff)
{
  float cxa, cya, ca, sa;
  cxa = calc_cd(polares, aoa) * cd_coeff;
  cya = calc_cl(polares, aoa) + cl_add;
  sincos(DegToRad(ang), sa, ca);
  float cx = cxa * ca - cya * sa;
  cx *= polares.kq;
  float cy = cya * ca + cxa * sa;
  cy *= polares.clKq;
  return Point2(cx, cy);
}

/** Get Cya from angle of attack (aoa) */
float gamephys::calc_cl(const Polares &polares, float aoa)
{
  // Linear part
  if (aoa <= polares.aoaLineH && aoa >= polares.aoaLineL)
    return polares.cl0 + polares.clLineCoeff * aoa * polares.cyMult;
  const float sign = fsel(aoa - polares.aoaLineH + 0.01f, 1.f, -1.f);
  const float aoaCrit = fsel(sign, polares.aoaCritH, polares.aoaCritL);
  const float clAfterCrit = fsel(sign, polares.clAfterCritH, polares.clAfterCritL);
  const float cyCrit = fsel(sign, polares.cyCritH, polares.cyCritL);
  // Parabolic part before crit
  if (sign * (aoa - aoaCrit) <= 0.f) // |aoa| <= |aoaCrit|
  {
    const float parabCyCoeff = fsel(sign, polares.parabCyCoeffH, polares.parabCyCoeffL);
    return cyCrit - sign * parabCyCoeff * sqr((aoaCrit - aoa));
  }
  float maxAng = max(40.f, polares.maxDistAng);
  if (sign * aoa <= maxAng)
  {
    if (sign * aoa <= polares.maxDistAng)
    {
      float dA = aoa - aoaCrit;
      if (sign * dA < polares.parabAngle)
        return cyCrit - sign * polares.declineCoeff * sqr(dA);
      else
      {
        float h = clAfterCrit * sinf(PI * 0.0125f * polares.maxDistAng);
        float maxDA = (sign * (polares.maxDistAng - polares.parabAngle) - aoaCrit);
        float needInc = cyCrit - sign * polares.declineCoeff * sqr(polares.parabAngle) - h;
        float parabCoeff = safediv(needInc, sqr(maxDA));
        return h + parabCoeff * sqr(sign * polares.maxDistAng - aoa);
      }
    }
    else
      return clAfterCrit * sinf(PI * 0.0125f * sign * aoa);
  }
  else if (sign * aoa <= 140.0f)
  { // angles close to maxAng + 50 (maxAng - (maxAng + 100))
    float localSign = sign;
    float localAOA = sign * aoa;
    if (sign * aoa > 90.0f)
    {
      localSign *= -1.f;
      localAOA = 40.0f + (140.0f - localAOA);
    }
    float sine = sign * clAfterCrit *
                 (sign * lerp(sinf(PI * 0.0125f * maxAng) - sinf(PI * 0.5f + PI * 0.01f * max(maxAng - 40.0f, 0.0f)), 0.0f,
                           (sign * aoa - 40.0f) / (140.0f - 40.0f)) +
                   sinf(PI * 0.5f + PI * 0.01f * (localAOA - 40.0f)) * localSign);
    return sine;
  }
  else
  { // Inverted flight (maxAng + 100..180)
    const float localSign = -1.f * sign;
    float localAOA = sign * aoa;
    localAOA = 180.0f - localAOA;
    const float sine = sign * clAfterCrit * sinf(PI * 0.0125f * localAOA); // 40.0f -> 0.5f *PI
    return sine * localSign;
  }
}

/** Get Cxa from angle of attack (aoa) */

float gamephys::calc_cd(const Polares &polares, float aoa)
{
  const float cy = polares.cl0 + polares.clLineCoeff * aoa;
  float indDrag = polares.cd0 + sqr(cy) * polares.indCoeff;
  const float sign = fsel(aoa, 1.f, -1.f);
  const float aoaCrit = fsel(aoa, polares.aoaCritH, polares.aoaCritL);
  indDrag += fsel(sign * (aoa - aoaCrit), polares.cdAfterCoeff * sign * (aoa - aoaCrit), 0.f);
  const float sin = 0.15f + polares.cyCritH * rabs(sinf(DegToRad(aoa)));
  return min(indDrag, sin);
}

/** Get Cz from sliding angle (AOS) */

float gamephys::calc_cs(const Polares & /*polares*/, float aos) { return 0.7f * ((float)sin(DegToRad(aos))); }

bool gamephys::calc_aoa(const Polares &polares, float cy, float &out_aoa)
{
  float cya = cy / polares.clKq;
  if (cya > polares.cyCritH)
  {
    out_aoa = polares.aoaCritH;
    return false;
  }
  else if (cya < polares.cyCritL)
  {
    out_aoa = polares.aoaCritL;
    return false;
  }
  const float alpha = safediv(cya - polares.cl0, polares.clLineCoeff * polares.cyMult);
  if (alpha <= polares.aoaLineH && alpha >= polares.aoaLineL)
    out_aoa = alpha;
  else
  {
    const float sign = fsel(alpha - polares.aoaLineH + 0.01f, 1.f, -1.f);
    const float aoaCrit = fsel(sign, polares.aoaCritH, polares.aoaCritL);
    const float cyCrit = fsel(sign, polares.cyCritH, polares.cyCritL);
    const float parabCyCoeff = fsel(sign, polares.parabCyCoeffH, polares.parabCyCoeffL);
    out_aoa = aoaCrit - sign * safe_sqrt(safediv(rabs(cyCrit - cya), parabCyCoeff));
  }
  return true;
}

struct CalcCy
{
  const Polares &polares;
  float cy;
  float ang;
  CalcCy(const Polares &polares_in, float cy_in, float ang_in) : polares(polares_in), cy(cy_in), ang(ang_in) {}
  inline float operator()(float aoa) const { return gamephys::calc_c(polares, aoa, aoa - ang).y - cy; }
};

bool gamephys::calc_aoa(const Polares &polares, float cy, float ang, float &out_aoa)
{
  if (cy > polares.cyCritH * polares.clKq - 1.0E-3f)
  {
    out_aoa = polares.aoaCritH;
    return false;
  }
  else if (cy < polares.cyCritL * polares.clKq + 1.0E-3f)
  {
    out_aoa = polares.aoaCritL;
    return false;
  }
  else
  {
    const CalcCy calcCy = {polares, cy, ang};
    float aoa = 0.0f;
    if (solve_newton(calcCy, 0.0f, 0.01f, 0.01f, 15u, aoa))
    {
      out_aoa = aoa;
      return true;
    }
    else
      return false;
  }
}

////////////////////

void gamephys::reset(gamephys::PolaresProps &props)
{
  props.lambda = 3.f;
  props.clLineCoeff = 0.85f;

  props.parabAngle = 5.0f;
  props.declineCoeff = 0.007f;
  props.maxDistAng = 30.0f;
  props.clAfterCritL = -1.09f;
  props.clAfterCritH = 1.09f;

  props.cl0 = 0.0f;
  props.aoaCritH = 16.0f;
  props.aoaCritL = -16.0f;
  props.clCritH = 1.0f;
  props.clCritL = -1.0f;
  props.cd0 = 0.02f;
  props.cdAfterCoeff = 0.01f;

  // Mach factor
  props.machFactorMode = PolaresProps::MACH_FACTOR_MODE_NONE;

  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CX0].machCrit = 0.6f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CX0].machMax = 1.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CX0].multMachMax = 7.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CX0].multLineCoeff = -5.2f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CX0].multLimit = 1.0f;

  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AUX].machCrit = 0.65f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AUX].machMax = 0.97f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AUX].multMachMax = 6.7f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AUX].multLineCoeff = -3.7f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AUX].multLimit = 1.0f;

  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CY_MAX].machCrit = 0.3f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CY_MAX].machMax = 1.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CY_MAX].multMachMax = 0.32f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CY_MAX].multLineCoeff = -0.44f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CY_MAX].multLimit = 0.25f;

  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AOA_CRIT].machCrit = 0.3f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AOA_CRIT].machMax = 1.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AOA_CRIT].multMachMax = 0.4f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AOA_CRIT].multLineCoeff = -0.2f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AOA_CRIT].multLimit = 0.25f;

  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_IND_COEFF].machCrit = 0.6f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_IND_COEFF].machMax = 1.5f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_IND_COEFF].multMachMax = 2.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_IND_COEFF].multLineCoeff = 1.1f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_IND_COEFF].multLimit = 5.0f;

  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AER_CENTER_OFFSET].machCrit = 0.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AER_CENTER_OFFSET].machMax = 0.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AER_CENTER_OFFSET].multMachMax = 0.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AER_CENTER_OFFSET].multLineCoeff = 0.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_AER_CENTER_OFFSET].multLimit = 0.0f;

  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CY0].machCrit = 0.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CY0].machMax = 1.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CY0].multMachMax = 1.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CY0].multLineCoeff = 0.0f;
  props.machFactor[PolaresProps::MACH_FACTOR_CURVE_CY0].multLimit = 1.0f;

  props.combinedCl = true;

  props.clToCmByMach.clear();
  props.clToCmByMach.add(0.0f, Point2(0.0f, 0.0f));

  props.span = 0.0f;
  props.area = 0.0f;
}

gamephys::PolaresProps gamephys::defaultPolarProps;

static void load_save_override_blk_polar_1(gamephys::PolaresProps &props, DataBlock *blk, float span, float area, float cl_to_cm,
  bool load)
{
  float oswEffNumber = props.lambda * safediv(props.area, sqr(props.span));
  blkutil::loadSaveBlk(blk, "OswaldsEfficiencyNumber", oswEffNumber, load);
  props.lambda = oswEffNumber * safediv(sqr(span), area);

  blkutil::loadSaveBlk(blk, "lineClCoeff", props.clLineCoeff, load);

  blkutil::loadSaveBlk(blk, "AfterCritParabAngle", props.parabAngle, load);
  blkutil::loadSaveBlk(blk, "AfterCritDeclineCoeff", props.declineCoeff, load);
  blkutil::loadSaveBlk(blk, "AfterCritMaxDistanceAngle", props.maxDistAng, load);
  blkutil::loadSaveBlk(blk, "CxAfterCoeff", props.cdAfterCoeff, 0.01f, load);

  if (load)
    props.clAfterCritH = blk->getReal("ClAfterCrit", props.clAfterCritH);
  blkutil::loadSaveBlk(blk, "ClAfterCritHigh", props.clAfterCritH, props.clAfterCritH, load);
  blkutil::loadSaveBlk(blk, "ClAfterCritLow", props.clAfterCritL, -props.clAfterCritH, load);

  blkutil::loadSaveBlkEnum(blk, "MachFactor", props.machFactorMode, load);
  for (unsigned int i = 0; i < gamephys::PolaresProps::MACH_FACTOR_CURVE_MAX; ++i)
  {
    gamephys::PolaresProps::MachFactor &machFactor = props.machFactor[i];
    const int index = i + 1;
    blkutil::loadSaveBlk(blk, String(32, "MachCrit%d", index), machFactor.machCrit, load);
    blkutil::loadSaveBlk(blk, String(32, "MachMax%d", index), machFactor.machMax, load);
    blkutil::loadSaveBlk(blk, String(32, "MultMachMax%d", index), machFactor.multMachMax, load);
    blkutil::loadSaveBlk(blk, String(32, "MultLineCoeff%d", index), machFactor.multLineCoeff, load);
    blkutil::loadSaveBlk(blk, String(32, "MultLimit%d", index), machFactor.multLimit, load);
  }
  blkutil::loadSaveBlk(blk, "CombinedCl", props.combinedCl, load);

  load_save_override_interpolate_table(props.clToCmByMach, *blk, "ClToCmByMach", Point3(1.0f, 1.0f, 1.0f), PolaresProps::MAX_CL_TO_CM,
    load);

  if (load && props.clToCmByMach.empty())
    props.clToCmByMach.add(0.0f, Point2(0.0f, cl_to_cm));

  if (load)
  {
    props.span = span;
    props.area = area;
  }
}

static void load_save_override_blk_polar_2(gamephys::PolaresProps &props, DataBlock *blk, bool load)
{
  blkutil::loadSaveBlk(blk, "Cl0", props.cl0, load);
  blkutil::loadSaveBlk(blk, "alphaCritHigh", props.aoaCritH, load);
  blkutil::loadSaveBlk(blk, "alphaCritLow", props.aoaCritL, load);
  blkutil::loadSaveBlk(blk, "ClCritHigh", props.clCritH, load);
  blkutil::loadSaveBlk(blk, "ClCritLow", props.clCritL, load);
  blkutil::loadSaveBlk(blk, "CdMin", props.cd0, load);
}

void gamephys::load_save_override_blk(gamephys::PolaresProps &props, DataBlock *blk1, DataBlock *blk2, float span, float area,
  float cl_to_cm, bool load)
{
  load_save_override_blk_polar_1(props, blk1, span, area, cl_to_cm, load);
  load_save_override_blk_polar_2(props, blk2, load);
  gamephys::recalc_mach_factor(props);
}

int gamephys::apply_modifications_1(gamephys::PolaresProps &props, const DataBlock &modBlk, const char *suffix, float mod_effect,
  float whole_mod_effect, float aspect_ratio)
{
  int numChanges = 0;

  Tab<ModRec> modDescr(framemem_ptr());
  registerMod(modBlk, String(32, "newCdMin%s", suffix).str(), ModRec::EM_SET, props.cd0, modDescr, whole_mod_effect);
  registerMod(modBlk, String(32, "newClLine%s", suffix).str(), ModRec::EM_SET, props.clLineCoeff, modDescr, whole_mod_effect);
  registerMod(modBlk, String(32, "newClCritH%s", suffix).str(), ModRec::EM_SET, props.clCritH, modDescr, whole_mod_effect);
  registerMod(modBlk, String(32, "newClCritL%s", suffix).str(), ModRec::EM_SET, props.clCritL, modDescr, whole_mod_effect);
  registerMod(modBlk, String(32, "newAlphaCritH%s", suffix).str(), ModRec::EM_SET, props.aoaCritH, modDescr, whole_mod_effect);
  registerMod(modBlk, String(32, "newAlphaCritL%s", suffix).str(), ModRec::EM_SET, props.aoaCritL, modDescr, whole_mod_effect);
  registerMod(modBlk, String(32, "newDeclineCoeff%s", suffix).str(), ModRec::EM_SET, props.declineCoeff, modDescr, whole_mod_effect);
  registerMod(modBlk, String(32, "mulOswEffNumber%s", suffix).str(), ModRec::EM_MUL, props.lambda, modDescr, mod_effect);

  const int newOswEffNumberId = modBlk.getNameId("newOswEffNumber");

  for (int i = 0; i < modBlk.paramCount(); ++i)
  {
    const int nid = modBlk.getParamNameId(i);

    for (int j = 0; j < modDescr.size(); ++j)
    {
      if (modDescr[j].type == ModRec::EM_SET &&
          setFromBlkIfFoundCounter(modBlk, nid, modDescr[j].nameId, i, modDescr[j].ref, numChanges))
        break;
      if (modDescr[j].type == ModRec::EM_MUL &&
          mulFromBlkIfFoundCounter(modBlk, nid, modDescr[j].nameId, i, modDescr[j].ref, numChanges, modDescr[j].modEffMult))
        break;
      if (modDescr[j].type == ModRec::EM_ADD &&
          addFromBlkIfFoundCounter(modBlk, nid, modDescr[j].nameId, i, modDescr[j].ref, numChanges, modDescr[j].modEffMult))
        break;
    }
    if (newOswEffNumberId == nid)
    {
      props.lambda = modBlk.getReal(i) * aspect_ratio;
      numChanges++;
    }
  }
  return numChanges;
}

int gamephys::apply_modifications_2_no_flaps(gamephys::PolaresProps &props, const DataBlock &modBlk, const char *suffix,
  float /*mod_effect*/, float whole_mod_effect, float /*aspect_ratio*/)
{
  int numChanges = 0;
  const int mulCdminId = modBlk.getNameId(String(32, "mulCdmin%s", suffix).str());
  for (int i = 0; i < modBlk.paramCount(); ++i)
  {
    const int nid = modBlk.getParamNameId(i);
    if (mulCdminId == nid)
    {
      float mulForCd = 1.f + (modBlk.getReal(i) - 1.f) * whole_mod_effect;
      props.cd0 *= mulForCd;
      numChanges++;
    }
  }
  return numChanges;
}

void gamephys::interpolate(const gamephys::PolaresProps &a, const gamephys::PolaresProps &b, float k, gamephys::PolaresProps &out)
{
  // normal polar
  out.lambda = lerp(a.lambda, b.lambda, k);
  out.clLineCoeff = lerp(a.clLineCoeff, b.clLineCoeff, k);

  // after crit polar
  out.parabAngle = lerp(a.parabAngle, b.parabAngle, k);
  out.declineCoeff = lerp(a.declineCoeff, b.declineCoeff, k);
  out.maxDistAng = lerp(a.maxDistAng, b.maxDistAng, k);
  out.cdAfterCoeff = lerp(a.cdAfterCoeff, b.cdAfterCoeff, k);
  out.clAfterCritL = lerp(a.clAfterCritL, b.clAfterCritL, k);
  out.clAfterCritH = lerp(a.clAfterCritH, b.clAfterCritH, k);

  // cl / cd
  out.cl0 = lerp(a.cl0, b.cl0, k);
  out.aoaCritH = lerp(a.aoaCritH, b.aoaCritH, k);
  out.aoaCritL = lerp(a.aoaCritL, b.aoaCritL, k);
  out.clCritH = lerp(a.clCritH, b.clCritH, k);
  out.clCritL = lerp(a.clCritL, b.clCritL, k);
  out.cd0 = lerp(a.cd0, b.cd0, k);

  // others
  out.span = lerp(a.span, b.span, k);
  out.area = lerp(a.area, b.area, k);

  // mach factor
  out.machFactorMode = k < 0.5f ? a.machFactorMode : b.machFactorMode;
  for (int i = 0; i < PolaresProps::MACH_FACTOR_CURVE_MAX; ++i)
  {
    out.machFactor[i].machCrit = lerp(a.machFactor[i].machCrit, b.machFactor[i].machCrit, k);
    out.machFactor[i].machMax = lerp(a.machFactor[i].machMax, b.machFactor[i].machMax, k);
    out.machFactor[i].multMachMax = lerp(a.machFactor[i].multMachMax, b.machFactor[i].multMachMax, k);
    out.machFactor[i].multLineCoeff = lerp(a.machFactor[i].multLineCoeff, b.machFactor[i].multLineCoeff, k);
    out.machFactor[i].multLimit = lerp(a.machFactor[i].multLimit, b.machFactor[i].multLimit, k);
    for (int j = 0; j < 4; ++j)
      out.machFactor[i].machCoeffs[j] = lerp(a.machFactor[i].machCoeffs[j], b.machFactor[i].machCoeffs[j], k);
  }
  out.combinedCl = k < 0.5f ? a.combinedCl : b.combinedCl;

  // Cl to Cm
  out.clToCmByMach.clear();
  for (int i = 0; i < a.clToCmByMach.size(); ++i)
  {
    const float arg = a.clToCmByMach[i].x;
    const Point2 &aVal = a.clToCmByMach[i].y;
    const Point2 bVal = b.clToCmByMach.interpolate(arg);
    out.clToCmByMach.add(arg, Point2(lerp(aVal.x, bVal.x, k), lerp(aVal.y, bVal.y, k)));
  }
}

static const carray<float, gamephys::PolaresProps::MACH_FACTOR_CURVE_MAX> preCritVal = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f};

void gamephys::recalc_mach_factor(gamephys::PolaresProps &props)
{
  for (unsigned int index = 0; index < PolaresProps::MACH_FACTOR_CURVE_MAX; ++index)
  {
    const float machCrit = props.machFactor[index].machCrit;
    const float machMax = props.machFactor[index].machMax;
    const float multMachMax = props.machFactor[index].multMachMax;
    const float multLineCoeff = props.machFactor[index].multLineCoeff;
    carray<float, 4> &machCoeffs = props.machFactor[index].machCoeffs;
    // F'(Mcrit)           F'(Mmax)             F(Mcrit)            F(Mmax)
    const TMatrix4 A(0.0f, 0.0f, 1.0f, 1.0f,                                         // machCoeffs[0]
      1.0f, 1.0f, machCrit, machMax,                                                 // machCoeffs[1]
      2.0f * machCrit, 2.0f * machMax, pow(machCrit, 2), pow(machMax, 2),            // machCoeffs[2]
      3.0f * sqr(machCrit), 3.0f * sqr(machMax), pow(machCrit, 3), pow(machMax, 3)); // machCoeffs[3]
    const Point4 Y(0.0f, multLineCoeff, preCritVal[index], multMachMax);
    TMatrix4 invA;
    float det = 0.0f;
    if (inverse44(A, invA, det))
    {
      const Point4 X = Y * invA;
      for (unsigned int i = 0; i < 4; ++i)
        machCoeffs[i] = X[i];
    }
    else
    {
      mem_set_0(machCoeffs);
      machCoeffs[3] = preCritVal[index];
    }
  }
}


template <unsigned int size>
inline float poly(const carray<float, size>(&coeffs), float v)
{
  return poly_impl<size>(coeffs.data(), v);
}

static float calc_mach_mult(const PolaresProps &props, float mach, unsigned int index)
{
  if (props.machFactorMode != gamephys::PolaresProps::MACH_FACTOR_MODE_ADVANCED)
    return preCritVal[index];
  else if (mach < props.machFactor[index].machCrit)
    return preCritVal[index];
  else if (mach > props.machFactor[index].machMax)
  {
    const float mult =
      props.machFactor[index].multMachMax + props.machFactor[index].multLineCoeff * (mach - props.machFactor[index].machMax);
    return fsel(props.machFactor[index].multLineCoeff, min(mult, props.machFactor[index].multLimit),
      max(mult, props.machFactor[index].multLimit));
  }
  else
    return poly(props.machFactor[index].machCoeffs, mach);
}

float gamephys::calc_mach_mult_derrivative(const PolaresProps &props, float mach, unsigned int index)
{
  if (props.machFactorMode != PolaresProps::MACH_FACTOR_MODE_ADVANCED)
    return 0.0f;
  else if (mach < props.machFactor[index].machCrit)
    return 0.0f;
  else if (mach > props.machFactor[index].machMax)
  {
    const float mult =
      props.machFactor[index].multMachMax + props.machFactor[index].multLineCoeff * (mach - props.machFactor[index].machMax);
    if (props.machFactor[index].multLineCoeff > 0.0f)
      return mult < props.machFactor[index].multLimit ? props.machFactor[index].multLineCoeff : 0.0f;
    else
      return mult > props.machFactor[index].multLimit ? props.machFactor[index].multLineCoeff : 0.0f;
  }
  else
  {
    const carray<float, 4> &machCoeffs = props.machFactor[index].machCoeffs;
    return 3.0f * machCoeffs[3] * sqr(mach) + 2.0f * machCoeffs[2] * mach + machCoeffs[1];
  }
}

static void prepare_polares(gamephys::Polares &in_out_data)
{
  // Update linear part
  in_out_data.aoaLineH =
    2.0f * safediv(in_out_data.cyCritH - in_out_data.cl0, in_out_data.clLineCoeff * in_out_data.cyMult) - in_out_data.aoaCritH;
  in_out_data.aoaLineL =
    2.0f * safediv(in_out_data.cyCritL - in_out_data.cl0, in_out_data.clLineCoeff * in_out_data.cyMult) - in_out_data.aoaCritL;
  if (in_out_data.aoaLineL > in_out_data.aoaLineH)
    in_out_data.aoaLineL = in_out_data.aoaLineH = 0.5f * (in_out_data.aoaLineL + in_out_data.aoaLineH);
  // Update parabolic part
  in_out_data.parabCyCoeffH =
    safediv(in_out_data.cyCritH - (in_out_data.cl0 + in_out_data.aoaLineH * in_out_data.clLineCoeff * in_out_data.cyMult),
      sqr(in_out_data.aoaCritH - in_out_data.aoaLineH));
  in_out_data.parabCyCoeffL =
    safediv(-in_out_data.cyCritL + (in_out_data.cl0 + in_out_data.aoaLineL * in_out_data.clLineCoeff * in_out_data.cyMult),
      sqr(in_out_data.aoaLineL - in_out_data.aoaCritL));
}

static void fill_polares_constants(const gamephys::PolaresProps &props, gamephys::Polares &out_polares)
{
  out_polares.parabAngle = props.parabAngle;
  out_polares.declineCoeff = props.declineCoeff;
  out_polares.maxDistAng = props.maxDistAng;
  out_polares.cdAfterCoeff = props.cdAfterCoeff;
  out_polares.clAfterCritL = props.clAfterCritL;
  out_polares.clAfterCritH = props.clAfterCritH;
}

void gamephys::fill_polares(const gamephys::PolaresProps &props, gamephys::Polares &out_polares)
{
  // multipliers
  out_polares.cyMult = 1.0f;

  // variable
  out_polares.cl0 = props.cl0;
  out_polares.cd0 = props.cd0;
  out_polares.indCoeff = safeinv(PI * props.lambda);
  out_polares.clLineCoeff = props.clLineCoeff;
  out_polares.cyCritH = props.clCritH;
  out_polares.cyCritL = props.clCritL;
  out_polares.aoaCritH = props.aoaCritH;
  out_polares.aoaCritL = props.aoaCritL;
  prepare_polares(out_polares);
  out_polares.aerCenterOffset = 0.0f;
  out_polares.clToCm.zero();

  // constant
  fill_polares_constants(props, out_polares);

  // mach multipliers
  out_polares.kq = 1.0f;
  out_polares.clKq = 1.0f;
}

static void calc_mach_dependent_polares(const gamephys::PolaresProps &props, float cl, float cl_crih_h, float cl_crih_l,
  float aoa_crit_h, float aoa_crit_l, float cd, float mach, gamephys::Polares &out_polares)
{
  // mach
  const float indCoeff0 = safeinv(PI * props.lambda);
  if (props.machFactorMode == PolaresProps::MACH_FACTOR_MODE_ADVANCED)
  {
    // Cx0(M)
    out_polares.cd0 = cd * calc_mach_mult(props, mach, PolaresProps::MACH_FACTOR_CURVE_CX0);
    // dCy/dAoA(M)
    out_polares.clLineCoeff =
      props.clLineCoeff * (props.combinedCl ? (1.0f + calc_mach_mult(props, mach, PolaresProps::MACH_FACTOR_CURVE_CX0) -
                                                calc_mach_mult(props, mach, PolaresProps::MACH_FACTOR_CURVE_AUX))
                                            : calc_mach_mult(props, mach, PolaresProps::MACH_FACTOR_CURVE_CL_LINE_COEFF));
    // Cy
    out_polares.cl0 = cl * calc_mach_mult(props, mach, PolaresProps::MACH_FACTOR_CURVE_CY0);
    // Cy crit
    const float cyCritMult = calc_mach_mult(props, mach, PolaresProps::MACH_FACTOR_CURVE_CY_MAX);
    out_polares.cyCritL = cl + (cl_crih_l - out_polares.cl0) * cyCritMult;
    out_polares.cyCritH = cl + (cl_crih_h - out_polares.cl0) * cyCritMult;
    // AOA crit
    const float aoaCritMult = calc_mach_mult(props, mach, PolaresProps::MACH_FACTOR_CURVE_AOA_CRIT);
    out_polares.aoaCritL = aoa_crit_l * aoaCritMult;
    out_polares.aoaCritH = aoa_crit_h * aoaCritMult;
    // Induction coeff
    const float indCoeffMult = calc_mach_mult(props, mach, PolaresProps::MACH_FACTOR_CURVE_IND_COEFF);
    out_polares.indCoeff = indCoeff0 * indCoeffMult;
    // Aerodynamic center offset
    out_polares.aerCenterOffset = calc_mach_mult(props, mach, PolaresProps::MACH_FACTOR_CURVE_AER_CENTER_OFFSET);
    out_polares.clToCm = props.clToCmByMach.interpolate(mach);

    out_polares.kq = out_polares.clKq = 1.0f;
  }
  else
  {
    out_polares.cd0 = cd;

    out_polares.cl0 = cl;
    out_polares.clLineCoeff = props.clLineCoeff;
    out_polares.indCoeff = indCoeff0;
    // Cy crit
    out_polares.cyCritL = cl_crih_l;
    out_polares.cyCritH = cl_crih_h;
    // AOA crit
    out_polares.aoaCritL = aoa_crit_l;
    out_polares.aoaCritH = aoa_crit_h;
    // Aerodynamic center offset
    out_polares.aerCenterOffset = 0.0f;
    out_polares.clToCm.zero();

    if (props.machFactorMode == PolaresProps::MACH_FACTOR_MODE_SIMPLE)
    {
      const float mach_lim = min(mach, 0.9f);
      out_polares.kq = 1.f / max(powf(min(rabs(1.f - sqr(mach_lim)), 1.f), 0.3f), 0.2f);
      out_polares.clKq = sqrtf(out_polares.kq);
    }
    else if (props.machFactorMode == PolaresProps::MACH_FACTOR_MODE_SIMPLE_PROPELLER)
    {
      out_polares.kq = 1.f / max(powf(rabs(1.f - sqr(mach)), 0.3f), 0.2f);
      out_polares.clKq = sqrtf(out_polares.kq);
    }
    else
      out_polares.kq = out_polares.clKq = 1.0f;
  }
  prepare_polares(out_polares);
}

void gamephys::calc_polares(const gamephys::PolaresProps &props, float mach, gamephys::Polares &out_polares)
{
  out_polares.cyMult = 1.0f;
  calc_mach_dependent_polares(props, props.cl0, props.clCritH, props.clCritL, props.aoaCritH, props.aoaCritL, props.cd0, mach,
    out_polares);
  fill_polares_constants(props, out_polares);
}

void gamephys::calc_polares(const gamephys::PolaresProps &props, float cy_mult, float mach, gamephys::Polares &out_polares)
{
  out_polares.cyMult = cy_mult;
  calc_mach_dependent_polares(props, props.cl0, props.clCritH * cy_mult, props.clCritL * cy_mult, props.aoaCritH, props.aoaCritL,
    props.cd0, mach, out_polares);
  fill_polares_constants(props, out_polares);
}