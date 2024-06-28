// Local changes for cc-lib:
// In here are the two header files that make up BiteOpt:
// (biteaux.h and biteopt.h).
// I added a random seed parameter to biteopt_minimize so that
// if we run it in a loop, we don't always do the same thing.
// I initialized
//    double s[ MaxParPopCount ] = {};
// to suppress a warning (code looked ok).
//
// In wrapParamReal, I changed
//   ( maxv - rnd.getRndValue() * ( v - dv ));
// to
//   ( maxv - rnd.getRndValue() * ( v - maxv ));
// as I believe was intended (otherwise it generates samples
// outside the range). See: github.com/avaneev/biteopt/issues/6
//
// Fixed some warnings (const cast, etc.)
//
// At the bottom are implementations of my wrapper in opt.h.


//$ nocpp

/**
 * @file biteaux.h
 *
 * @brief The inclusion file for the CBiteRnd, CBiteOptPop, CBiteOptParPops,
 * CBiteOptInterface, and CBiteOptBase classes.
 *
 * @section license License
 *
 * Copyright (c) 2016-2021 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @version 2021.28
 */

#ifndef BITEAUX_INCLUDED
#define BITEAUX_INCLUDED

#include <stdint.h>
#include <math.h>
#include <string.h>

/**
 * Class that implements a pseudo-random number generator (PRNG). The default
 * implementation includes a relatively fast PRNG that uses a classic formula
 * "seed = ( a * seed + c ) % m" (LCG), in a rearranged form. This
 * implementation uses bits 34-63 (30 bits) of the state variable which
 * ensures at least 2^34 period in the lowest significant bit of the resulting
 * pseudo-random sequence. See
 * https://en.wikipedia.org/wiki/Linear_congruential_generator for more
 * details.
 */

class CBiteRnd
{
public:
  /**
   * Default constructor, calls the init() function.
   */

  CBiteRnd()
  {
    init( 1 );
  }

  /**
   * Constructor, calls the init() function.
   *
   * @param NewSeed Random seed value.
   */

  CBiteRnd( const int NewSeed )
  {
    init( NewSeed );
  }

  /**
   * Function initializes *this PRNG object.
   *
   * @param NewSeed New random seed value.
   */

  void init( const int NewSeed )
  {
    BitsLeft = 0;
    Seed = (uint64_t) NewSeed;

    // Skip first values to make PRNG "settle down".

    advance();
    advance();
    advance();
    advance();
  }

  /**
   * @return Inverse scale of "raw" random values returned by functions with
   * the "raw" suffix.
   */

  inline static double getRawScaleInv()
  {
    static const double m = 0.5 / ( 1ULL << ( raw_bits - 1 ));

    return( m );
  }

  /**
   * @return Random number in the range [0; 1).
   */

  double getRndValue()
  {
    return( advance() * getRawScaleInv() );
  }

  /**
   * @return Random number in the range [0; 1), squared.
   */

  double getRndValueSqr()
  {
    const double v = advance() * getRawScaleInv();

    return( v * v );
  }

  /**
   * @return Uniformly-distributed random number in the "raw" scale.
   */

  uint32_t getUniformRaw()
  {
    return( advance() );
  }

  /**
   * @return Dual bit-size uniformly-distributed random number in the "raw"
   * scale.
   */

  uint64_t getUniformRaw2()
  {
    uint64_t v = (uint64_t) advance();
    v |= (uint64_t) advance() << raw_bits;

    return( v );
  }

  /**
   * @return TPDF random number in the range (-1; 1).
   */

  double getTPDF()
  {
    const int64_t v1 = (int64_t) advance();
    const int64_t v2 = (int64_t) advance();

    return(( v1 - v2 ) * getRawScaleInv() );
  }

  /**
   * Function returns the next random bit, usually used for 50% probability
   * evaluations efficiently.
   */

  int getBit()
  {
    if( BitsLeft == 0 )
    {
      BitPool = getUniformRaw();

      const int b = ( BitPool & 1 );

      BitsLeft = raw_bits - 1;
      BitPool >>= 1;

      return( b );
    }

    const int b = ( BitPool & 1 );

    BitsLeft--;
    BitPool >>= 1;

    return( b );
  }

private:
  static const int raw_bits = 29; ///< The number of higher bits used for
    ///< PRNG output.
    ///<
  static const int raw_shift = sizeof( uint64_t ) * 8 - raw_bits; ///<
    ///< "seed" value's bit shift to obtain the output value.
    ///<
  uint64_t Seed; ///< The current random seed value.
    ///<
  uint32_t BitPool; ///< Bit pool.
    ///<
  int BitsLeft; ///< The number of bits left in the bit pool.
    ///<

  /**
   * Function advances the PRNG and returns the next PRNG value.
   */

  uint32_t advance()
  {
    Seed = ( Seed + 15509ULL ) * 11627070389458151377ULL;

    return( (uint32_t) ( Seed >> raw_shift ));
  }
};

/**
 * Histogram class. Used to keep track of success of various choices. Updates
 * probabilities of future choices based on the current histogram state. Uses
 * a self-optimization technique for internal parameters.
 */

class CBiteOptHistBase
{
public:
  /**
   * Constructor.
   *
   * @param Count The number of possible choices, greater than 1.
   */

  CBiteOptHistBase( const int aCount )
    : Count( aCount )
    , m( 1.0 / aCount )
  {
  }

  /**
   * This function resets histogram, should be called before calling other
   * functions, including after object's construction.
   *
   * @param rnd PRNG object.
   */

  void reset( CBiteRnd& rnd )
  {
    const int b = rnd.getBit();
    IncrDecrHist[ 1 ] = b;
    IncrDecrHist[ 2 ] = 1 - b;
    IncrDecr = 2 - b;

    memset( Hist, 0, sizeof( Hist ));
    updateProbs();

    select( rnd );
  }

  /**
   * An auxiliary function that returns histogram's choice count.
   */

  int getChoiceCount() const
  {
    return( Count );
  }

  /**
   * This function should be called when a certain choice is successful.
   * This function should only be called after a prior select() calls.
   *
   * @param rnd PRNG object. May not be used.
   */

  void incr( CBiteRnd& rnd )
  {
    IncrDecrHist[ IncrDecr ]++;
    IncrDecr = 1 + ( IncrDecrHist[ 2 ] > IncrDecrHist[ 1 ]);

    Hist[ Sel ] += IncrDecr;
    updateProbs();
  }

  /**
   * This function should be called when a certain choice is a failure.
   * This function should only be called after a prior select() calls.
   *
   * @param rnd PRNG object. May not be used.
   */

  void decr( CBiteRnd& rnd )
  {
    IncrDecrHist[ IncrDecr ]--;
    IncrDecr = 1 + ( IncrDecrHist[ 2 ] > IncrDecrHist[ 1 ]);

    Hist[ Sel ] -= IncrDecr;
    updateProbs();
  }

  /**
   * Function produces a random choice index based on the current histogram
   * state. Note that "select" functions can only be called once for a given
   * histogram during the optimize() function call.
   *
   * @param rnd PRNG object.
   */

  int select( CBiteRnd& rnd )
  {
    const double rv = rnd.getUniformRaw() * ProbSum;
    int i;

    for( i = 0; i < Count - 1; i++ )
    {
      if( rv < Probs[ i ])
      {
        Sel = i;
        return( i );
      }
    }

    Sel = Count - 1;
    return( Count - 1 );
  }

  /**
   * Function returns the latest made choice index.
   */

  int getSel() const
  {
    return( Sel );
  }

protected:
  static const int MaxCount = 8; ///< The maximal number of choices
    ///< supported.
    ///<
  int Count; ///< The number of choices in use.
    ///<
  double m; ///< Multiplier (depends on Count).
    ///<
  int Hist[ MaxCount ]; ///< Histogram.
    ///<
  int IncrDecrHist[ 3 ]; ///< IncrDecr self-optimization histogram, element
    ///< 0 not used for efficiency.
    ///<
  int IncrDecr; ///< Histogram-driven increment or decrement, can be equal
    ///< to 1 or 2.
    ///<
  double Probs[ MaxCount ]; ///< Probabilities, cumulative.
    ///<
  double ProbSum; ///< Sum of probabilities, for random variable scaling.
    ///<
  int Sel; ///< The latest selected choice. Available only after the
    ///< select() function calls.
    ///<

  /**
   * Function updates probabilities of choices based on the histogram state.
   */

  void updateProbs()
  {
    int MinHist = Hist[ 0 ];
    int i;

    for( i = 1; i < Count; i++ )
    {
      if( Hist[ i ] < MinHist )
      {
        MinHist = Hist[ i ];
      }
    }

    MinHist--;
    double HistSum = 0.0;

    for( i = 0; i < Count; i++ )
    {
      Probs[ i ] = Hist[ i ] - MinHist;
      HistSum += Probs[ i ];
    }

    HistSum *= m * IncrDecr;
    ProbSum = 0.0;

    for( i = 0; i < Count; i++ )
    {
      const double v = ( Probs[ i ] < HistSum ? HistSum : Probs[ i ]) +
        ProbSum;

      Probs[ i ] = v;
      ProbSum = v;
    }

    ProbSum *= CBiteRnd :: getRawScaleInv();
  }
};

/**
 * Templated histogram class, for convenient constructor's Count parameter
 * specification.
 *
 * @tparam tCount The number of possible choices, greater than 1.
 */

template< int tCount >
class CBiteOptHist : public CBiteOptHistBase
{
public:
  CBiteOptHist()
    : CBiteOptHistBase( tCount )
  {
  }
};

/**
 * Class implements storage of population parameter vectors, costs, centroid,
 * and ordering.
 *
 * @tparam ptype Parameter value storage type.
 */

template< typename ptype >
class CBiteOptPop
{
public:
  CBiteOptPop()
    : ParamCount( 0 )
    , PopSize( 0 )
    , PopParamsBuf( NULL )
    , PopParams( NULL )
    , PopCosts( NULL )
    , CentParams( NULL )
    , NeedCentUpdate( false )
  {
  }

  CBiteOptPop( const CBiteOptPop& s )
    : PopParamsBuf( NULL )
    , PopParams( NULL )
    , PopCosts( NULL )
    , CentParams( NULL )
  {
    initBuffers( s.ParamCount, s.PopSize );
    copy( s );
  }

  virtual ~CBiteOptPop()
  {
    deleteBuffers();
  }

  CBiteOptPop& operator = ( const CBiteOptPop& s )
  {
    copy( s );
    return( *this );
  }

  /**
   * Function initializes all common buffers, and "PopSize" variables. This
   * function should be called when population's dimensions were changed.
   * This function calls the deleteBuffers() function to release any
   * derived classes' allocated buffers. Allocates an additional vector for
   * temporary use, which is at the same the last vector in the PopParams
   * array. Derived classes should call this function of the base class.
   *
   * @param aParamCount New parameter count.
   * @param aPopSize New population size. If <= 0, population buffers will
   * not be allocated.
   */

  virtual void initBuffers( const int aParamCount, const int aPopSize )
  {
    deleteBuffers();

    ParamCount = aParamCount;
    PopSize = aPopSize;
    CurPopSize = aPopSize;
    CurPopSize1 = aPopSize - 1;

    PopParamsBuf = new ptype[( aPopSize + 1 ) * aParamCount ];
    PopParams = new ptype*[ aPopSize + 1 ]; // Last element is temporary.
    PopCosts = new double[ aPopSize ];
    CentParams = new ptype[ aParamCount ];

    int i;

    for( i = 0; i <= aPopSize; i++ )
    {
      PopParams[ i ] = PopParamsBuf + i * aParamCount;
    }

    TmpParams = PopParams[ aPopSize ];
  }

  /**
   * Function copies population from the specified source population. If
   * *this population has a different size, or is uninitialized, it will
   * be initialized to source's population size.
   *
   * @param s Source population to copy. Should be initalized.
   */

  void copy( const CBiteOptPop& s )
  {
    if( ParamCount != s.ParamCount || PopSize != s.PopSize )
    {
      initBuffers( s.ParamCount, s.PopSize );
    }

    CurPopSize = s.CurPopSize;
    CurPopSize1 = s.CurPopSize1;
    CurPopPos = s.CurPopPos;
    NeedCentUpdate = s.NeedCentUpdate;

    int i;

    for( i = 0; i < CurPopSize; i++ )
    {
      memcpy( PopParams[ i ], s.PopParams[ i ],
        ParamCount * sizeof( PopParams[ i ][ 0 ]));
    }

    memcpy( PopCosts, s.PopCosts, CurPopSize * sizeof( PopCosts[ 0 ]));

    if( !NeedCentUpdate )
    {
      memcpy( CentParams, s.CentParams, ParamCount *
        sizeof( CentParams[ 0 ]));
    }
  }

  /**
   * Function recalculates centroid based on the current population size.
   * The NeedCentUpdate variable can be checked if centroid update is
   * needed. This function resets the NeedCentUpdate to "false".
   */

  void updateCentroid()
  {
    NeedCentUpdate = false;

    const int BatchCount = ( 1 << IntOverBits ) - 1;
    const double m = 1.0 / CurPopSize;
    ptype* const cp = CentParams;
    int i;
    int j;

    if( CurPopSize <= BatchCount )
    {
      memcpy( cp, PopParams[ 0 ], ParamCount * sizeof( cp[ 0 ]));

      for( j = 1; j < CurPopSize; j++ )
      {
        const ptype* const p = PopParams[ j ];

        for( i = 0; i < ParamCount; i++ )
        {
          cp[ i ] += p[ i ];
        }
      }

      for( i = 0; i < ParamCount; i++ )
      {
        cp[ i ] = (ptype) ( cp[ i ] * m );
      }
    }
    else
    {
      // Batched centroid calculation, for more precision and no integer
      // overflows.

      ptype* const tp = TmpParams;
      int pl = CurPopSize;
      j = 0;
      bool DoCopy = true;

      while( pl > 0 )
      {
        int c = ( pl > BatchCount ? BatchCount : pl );
        pl -= c;
        c--;

        memcpy( tp, PopParams[ j ], ParamCount * sizeof( tp[ 0 ]));

        while( c > 0 )
        {
          j++;
          const ptype* const p = PopParams[ j ];

          for( i = 0; i < ParamCount; i++ )
          {
            tp[ i ] += p[ i ];
          }

          c--;
        }

        if( DoCopy )
        {
          DoCopy = false;

          for( i = 0; i < ParamCount; i++ )
          {
            cp[ i ] = (ptype) ( tp[ i ] * m );
          }
        }
        else
        {
          for( i = 0; i < ParamCount; i++ )
          {
            cp[ i ] += (ptype) ( tp[ i ] * m );
          }
        }
      }
    }
  }

  /**
   * Function returns pointer to the centroid vector. The NeedUpdateCent
   * should be checked and and if it is equal to "true", the
   * updateCentroid() function called.
   */

  const ptype* getCentroid() const
  {
    return( CentParams );
  }

  /**
   * Function returns population's parameter vector by the specified index
   * from the ordered list.
   *
   * @param i Parameter vector index.
   */

  const ptype* getParamsOrdered( const int i ) const
  {
    return( PopParams[ i ]);
  }

  /**
   * Function returns a pointer to array of population vector pointers,
   * which are sorted in the ascending cost order.
   */

  const ptype* const* getPopParams() const
  {
    return( (const ptype* const*) PopParams );
  }

  /**
   * Function returns current population size.
   */

  int getCurPopSize() const
  {
    return( CurPopSize );
  }

  /**
   * Function returns current population position.
   */

  int getCurPopPos() const
  {
    return( CurPopPos );
  }

  /**
   * Function resets the current population position to zero. This function
   * is usually called when the population needs to be completely changed.
   * This function should be called before any updates to *this population
   * (usually during optimizer's initialization).
   */

  void resetCurPopPos()
  {
    CurPopPos = 0;
    NeedCentUpdate = false;
  }

  /**
   * Function returns "true" if the specified cost meets population's
   * cost constraint. The check is synchronized with the sortPop() function.
   *
   * @param Cost Cost value to evaluate.
   */

  bool isAcceptedCost( const double Cost ) const
  {
    return( Cost <= PopCosts[ CurPopSize1 ]);
  }

  /**
   * Function replaces the highest-cost previous solution, updates centroid.
   * This function considers the value of the CurPopPos variable - if it is
   * smaller than the CurPopSize, the new solution will be added to
   * population without any checks.
   *
   * @param NewCost Cost of the new solution.
   * @param UpdParams New parameter values.
   * @param DoUpdateCentroid "True" if centroid should be updated using
   * running sum. This update is done for parallel populations.
   * @param DoCostCheck "True" if the cost contraint should be checked.
   * Function returns "false" if the cost constraint was not met, "true"
   * otherwise.
   */

  bool updatePop( const double NewCost, const ptype* const UpdParams,
    const bool DoUpdateCentroid, const bool DoCostCheck )
  {
    if( CurPopPos < CurPopSize )
    {
      memcpy( PopParams[ CurPopPos ], UpdParams,
        ParamCount * sizeof( PopParams[ CurPopPos ][ 0 ]));

      sortPop( NewCost, CurPopPos );
      CurPopPos++;

      return( true );
    }

    if( DoCostCheck )
    {
      if( !isAcceptedCost( NewCost ))
      {
        return( false );
      }
    }

    ptype* const rp = PopParams[ CurPopSize1 ];

    if( DoUpdateCentroid )
    {
      ptype* const cp = CentParams;
      const double m = 1.0 / CurPopSize;
      int i;

      for( i = 0; i < ParamCount; i++ )
      {
        cp[ i ] += (ptype) (( UpdParams[ i ] - rp[ i ]) * m );
        rp[ i ] = UpdParams[ i ];
      }
    }
    else
    {
      memcpy( rp, UpdParams, ParamCount * sizeof( rp[ 0 ]));
      NeedCentUpdate = true;
    }

    sortPop( NewCost, CurPopSize1 );

    return( true );
  }

  /**
   * Function increases current population size, and updates the required
   * variables. This function can only be called if CurPopSize is less than
   * PopSize.
   *
   * @param CopyVec If >=, a parameter vector with this index will be copied
   * to the newly-added vector. The maximal population cost will be copied
   * as well.
   */

  void incrCurPopSize( const int CopyVec = -1 )
  {
    if( CopyVec >= 0 )
    {
      PopCosts[ CurPopSize ] = PopCosts[ CurPopSize1 ];
      memcpy( PopParams[ CurPopSize ], PopParams[ CopyVec ],
        ParamCount * sizeof( PopParams[ CurPopSize ][ 0 ]));
    }

    CurPopSize++;
    CurPopSize1++;
    NeedCentUpdate = true;
  }

  /**
   * Function decreases current population size, and updates the required
   * variables.
   */

  void decrCurPopSize()
  {
    CurPopSize--;
    CurPopSize1--;
    NeedCentUpdate = true;
  }

protected:
  static const int IntOverBits = ( sizeof( ptype ) > 4 ? 5 : 3 ); ///< The
    ///< number of bits of precision required for centroid calculation and
    ///< overflows.
    ///<
  static const int IntMantBits = sizeof( ptype ) * 8 - 1 - IntOverBits; ///<
    ///< Mantissa size of the integer parameter values (higher by 1 bit in
    ///< practice for real value 1.0). Accounts for a sign bit, and
    ///< possible overflows.
    ///<
  static const int64_t IntMantMult = 1LL << IntMantBits; ///< Mantissa
    ///< multiplier.
    ///<
  static const int64_t IntMantMultM = -IntMantMult; ///< Negative
    ///< IntMantMult.
    ///<
  static const int64_t IntMantMult2 = ( IntMantMult << 1 ); ///< =
    ///< IntMantMult * 2.
    ///<
  static const int64_t IntMantMask = IntMantMult - 1; ///< Mask that
    ///< corresponds to mantissa.
    ///<

  int ParamCount; ///< The total number of internal parameter values in use.
    ///<
  int PopSize; ///< The size of population in use (maximal).
    ///<
  int CurPopSize; ///< Current population size.
    ///<
  int CurPopSize1; ///< = CurPopSize - 1.
    ///<
  int CurPopPos; ///< Current population position, for initial population
    ///< update. This variable should be initialized by the optimizer.
    ///<
  ptype* PopParamsBuf; ///< Buffer for all PopParams vectors.
///<
  ptype** PopParams; ///< Population parameter vectors. Always kept sorted
    ///< in ascending cost order.
    ///<
  double* PopCosts; ///< Costs of population parameter vectors, sorting
    ///< order corresponds to PopParams.
    ///<
  ptype* CentParams; ///< Centroid of the current parameter vectors.
    ///<
  bool NeedCentUpdate; ///< "True" if centroid update is needed.
    ///<
  ptype* TmpParams; ///< Temporary parameter vector, points to the last
    ///< element of the PopParams array.
    ///<

  /**
   * Function deletes buffers previously allocated via the initBuffers()
   * function. Derived classes should call this function of the base class.
   */

  virtual void deleteBuffers()
  {
    delete[] PopParamsBuf;
    delete[] PopParams;
    delete[] PopCosts;
    delete[] CentParams;
  }

  /**
   * Function performs re-sorting of the population based on the cost of a
   * newly-added solution, and stores new cost.
   *
   * @param Cost Solution's cost.
   * @param i Solution's index (usually, CurPopSize1).
   */

  void sortPop( const double Cost, int i )
  {
    ptype* const InsertParams = PopParams[ i ];

    while( i > 0 )
    {
      const double c1 = PopCosts[ i - 1 ];

      if( c1 < Cost )
      {
        break;
      }

      PopCosts[ i ] = c1;
      PopParams[ i ] = PopParams[ i - 1 ];
      i--;
    }

    PopCosts[ i ] = Cost;
    PopParams[ i ] = InsertParams;
  }

  /**
   * Function wraps the specified parameter value so that it stays in the
   * [0.0; 1.0] range (including in integer range), by wrapping it over the
   * boundaries using random operator. This operation improves convergence
   * in comparison to clamping.
   *
   * @param v Parameter value to wrap.
   * @return Wrapped parameter value.
   */

  static ptype wrapParam( CBiteRnd& rnd, const ptype v )
  {
    if( (ptype) 0.25 == 0 )
    {
      if( v < 0 )
      {
        if( v > IntMantMultM )
        {
          return( (ptype) ( rnd.getRndValue() * -v ));
        }

        return( (ptype) ( rnd.getUniformRaw2() & IntMantMask ));
      }

      if( v > IntMantMult )
      {
        if( v < IntMantMult2 )
        {
          return( (ptype) ( IntMantMult -
            rnd.getRndValue() * ( v - IntMantMult )));
        }

        return( (ptype) ( rnd.getUniformRaw2() & IntMantMask ));
      }

      return( v );
    }
    else
    {
      if( v < 0.0 )
      {
        if( v > -1.0 )
        {
          return( (ptype) ( rnd.getRndValue() * -v ));
        }

        return( (ptype) rnd.getRndValue() );
      }

      if( v > 1.0 )
      {
        if( v < 2.0 )
        {
          return( (ptype) ( 1.0 - rnd.getRndValue() * ( v - 1.0 )));
        }

        return( (ptype) rnd.getRndValue() );
      }

      return( v );
    }
  }

  /**
   * Function generates a Gaussian-distributed pseudo-random number with
   * mean=0 and std.dev=1.
   *
   * @param rnd Uniform PRNG.
   */

  static double getGaussian( CBiteRnd& rnd )
  {
    double q, u, v;

    do
    {
      u = rnd.getRndValue();
      v = rnd.getRndValue();

      if( u <= 0.0 || v <= 0.0 )
      {
        u = 1.0;
        v = 1.0;
      }

      v = 1.7156 * ( v - 0.5 );
      const double x = u - 0.449871;
      const double y = fabs( v ) + 0.386595;
      q = x * x + y * ( 0.19600 * y - 0.25472 * x );

      if( q < 0.27597 )
      {
        break;
      }
    } while(( q > 0.27846 ) || ( v * v > -4.0 * log( u ) * u * u ));

    return( v / u );
  }

  /**
   * Function generates a Gaussian-distributed pseudo-random number, in
   * integer scale, with the specified mean and std.dev.
   *
   * @param rnd Uniform PRNG.
   * @param sd Standard deviation multiplier.
   * @param meanInt Mean value, in integer scale.
   */

  static ptype getGaussianInt( CBiteRnd& rnd, const double sd,
    const ptype meanInt )
  {
    while( true )
    {
      const double r = getGaussian( rnd ) * sd;

      if( r > -8.0 && r < 8.0 )
      {
        return( (ptype) ( r * IntMantMult + meanInt ));
      }
    }
  }
};

/**
 * Population class that embeds a dynamically-allocated parallel population
 * objects.
 *
 * @tparam ptype Parameter value storage type.
 */

template< typename ptype >
class CBiteOptParPops : virtual public CBiteOptPop< ptype >
{
public:
  CBiteOptParPops()
    : ParPopCount( 0 )
  {
  }

  virtual ~CBiteOptParPops()
  {
    int i;

    for( i = 0; i < ParPopCount; i++ )
    {
      delete ParPops[ i ];
    }
  }

protected:
  using CBiteOptPop< ptype > :: ParamCount;

  static const int MaxParPopCount = 8; ///< The maximal number of parallel
    ///< population supported.
    ///<
  CBiteOptPop< ptype >* ParPops[ MaxParPopCount ]; ///< Parallel
    ///< population orbiting *this population.
    ///<
  int ParPopCount; ///< Parallel population count. This variable should only
    ///< be changed via the setParPopCount() function.
    ///<

  /**
   * Function changes the parallel population count, and reallocates
   * buffers.
   *
   * @param NewCount New parallel population count, >= 0.
   */

  void setParPopCount( const int NewCount )
  {
    while( ParPopCount > NewCount )
    {
      ParPopCount--;
      delete ParPops[ ParPopCount ];
    }

    while( ParPopCount < NewCount )
    {
      ParPops[ ParPopCount ] = new CBiteOptPop< ptype >();
      ParPopCount++;
    }
  }

  /**
   * Function returns index of the parallel population that is most close
   * to the specified parameter vector. Function returns -1 if the cost
   * constraint is not met in all parallel populations.
   *
   * @param Cost Cost of parameter vector, used to filter considered
   * parallel population pool.
   * @param p Parameter vector.
   */

  int getMinDistParPop( const double Cost, const ptype* const p ) const
  {
    int ppi[ MaxParPopCount ];
    int ppc = 0;
    int i;

    for( i = 0; i < ParPopCount; i++ )
    {
      if( ParPops[ i ] -> isAcceptedCost( Cost ))
      {
        ppi[ ppc ] = i;
        ppc++;
      }
    }

    if( ppc == 0 )
    {
      return( -1 );
    }

    if( ppc == 1 )
    {
      return( ppi[ 0 ]);
    }

    double s[ MaxParPopCount ] = {};

    if( ppc == 4 ) {
      const ptype* const c0 = ParPops[ ppi[ 0 ]] -> getCentroid();
      const ptype* const c1 = ParPops[ ppi[ 1 ]] -> getCentroid();
      const ptype* const c2 = ParPops[ ppi[ 2 ]] -> getCentroid();
      const ptype* const c3 = ParPops[ ppi[ 3 ]] -> getCentroid();
      double s0 = 0.0;
      double s1 = 0.0;
      double s2 = 0.0;
      double s3 = 0.0;

      for( i = 0; i < ParamCount; i++ )
      {
        const ptype v = p[ i ];
        const double d0 = (double) ( v - c0[ i ]);
        const double d1 = (double) ( v - c1[ i ]);
        const double d2 = (double) ( v - c2[ i ]);
        const double d3 = (double) ( v - c3[ i ]);
        s0 += d0 * d0;
        s1 += d1 * d1;
        s2 += d2 * d2;
        s3 += d3 * d3;
      }

      s[ 0 ] = s0;
      s[ 1 ] = s1;
      s[ 2 ] = s2;
      s[ 3 ] = s3;
    } else if ( ppc == 3 ) {
      const ptype* const c0 = ParPops[ ppi[ 0 ]] -> getCentroid();
      const ptype* const c1 = ParPops[ ppi[ 1 ]] -> getCentroid();
      const ptype* const c2 = ParPops[ ppi[ 2 ]] -> getCentroid();
      double s0 = 0.0;
      double s1 = 0.0;
      double s2 = 0.0;

      for( i = 0; i < ParamCount; i++ )
      {
        const ptype v = p[ i ];
        const double d0 = (double) ( v - c0[ i ]);
        const double d1 = (double) ( v - c1[ i ]);
        const double d2 = (double) ( v - c2[ i ]);
        s0 += d0 * d0;
        s1 += d1 * d1;
        s2 += d2 * d2;
      }

      s[ 0 ] = s0;
      s[ 1 ] = s1;
      s[ 2 ] = s2;
    } else if (ppc == 2) {
      const ptype* const c0 = ParPops[ ppi[ 0 ]] -> getCentroid();
      const ptype* const c1 = ParPops[ ppi[ 1 ]] -> getCentroid();
      double s0 = 0.0;
      double s1 = 0.0;

      for( i = 0; i < ParamCount; i++ )
      {
        const ptype v = p[ i ];
        const double d0 = (double) ( v - c0[ i ]);
        const double d1 = (double) ( v - c1[ i ]);
        s0 += d0 * d0;
        s1 += d1 * d1;
      }

      s[ 0 ] = s0;
      s[ 1 ] = s1;
    }

    int pp = 0;
    double d = s[ pp ];

    for( i = 1; i < ppc; i++ )
    {
      if( s[ i ] <= d )
      {
        pp = i;
        d = s[ i ];
      }
    }

    return( ppi[ pp ]);
  }
};

/**
 * Base virtual abstract class that defines common optimizer interfacing
 * functions.
 */

class CBiteOptInterface
{
public:
  CBiteOptInterface()
  {
  }

  virtual ~CBiteOptInterface()
  {
  }

  /**
   * @return Best parameter vector.
   */

  virtual const double* getBestParams() const = 0;

  /**
   * @return Cost of the best parameter vector.
   */

  virtual double getBestCost() const = 0;

  /**
   * Virtual function that should fill minimal parameter value vector.
   *
   * @param[out] p Minimal value vector.
   */

  virtual void getMinValues( double* const p ) const = 0;

  /**
   * Virtual function that should fill maximal parameter value vector.
   *
   * @param[out] p Maximal value vector.
   */

  virtual void getMaxValues( double* const p ) const = 0;

  /**
   * Virtual function (objective function) that should calculate parameter
   * vector's optimization cost.
   *
   * @param p Parameter vector to evaluate.
   * @return Optimized cost.
   */

  virtual double optcost( const double* const p ) = 0;
};

/**
 * The base class for optimizers of the "biteopt" project.
 *
 * @tparam ptype Parameter value storage type.
 */

template< typename ptype >
class CBiteOptBase : public CBiteOptInterface,
  virtual protected CBiteOptParPops< ptype >
{
private:
  CBiteOptBase( const CBiteOptBase& )
  {
    // Copy-construction unsupported.
  }

  CBiteOptBase& operator = ( const CBiteOptBase& )
  {
    // Copying unsupported.
    return( *this );
  }

public:
  CBiteOptBase()
    : MinValues( NULL )
    , MaxValues( NULL )
    , DiffValues( NULL )
    , DiffValuesI( NULL )
    , BestValues( NULL )
    , NewValues( NULL )
    , HistCount( 0 )
  {
  }

  virtual const double* getBestParams() const
  {
    return( BestValues );
  }

  virtual double getBestCost() const
  {
    return( BestCost );
  }

  static const int MaxHistCount = 64; ///< The maximal number of histograms
    ///< that can be added (for static arrays).
    ///<

  /**
   * Function returns a pointer to an array of histograms in use.
   */

  CBiteOptHistBase** getHists()
  {
    return( Hists );
  }

  /**
   * Function returns a pointer to an array of histogram names.
   */

  const char* const* getHistNames() const
  {
    return( (const char* const*) HistNames );
  }

  /**
   * Function returns the number of histograms in use.
   */

  int getHistCount() const
  {
    return( HistCount );
  }

protected:
  using CBiteOptParPops< ptype > :: IntMantMult;
  using CBiteOptParPops< ptype > :: ParamCount;
  using CBiteOptParPops< ptype > :: PopSize;
  using CBiteOptParPops< ptype > :: CurPopSize;
  using CBiteOptParPops< ptype > :: CurPopSize1;
  using CBiteOptParPops< ptype > :: CurPopPos;
  using CBiteOptParPops< ptype > :: NeedCentUpdate;
  using CBiteOptParPops< ptype > :: resetCurPopPos;

  double* MinValues; ///< Minimal parameter values.
    ///<
  double* MaxValues; ///< Maximal parameter values.
    ///<
  double* DiffValues; ///< Difference between maximal and minimal parameter
    ///< values.
    ///<
  double* DiffValuesI; ///< Inverse DiffValues.
    ///<
  double* BestValues; ///< Best parameter vector.
    ///<
  double BestCost; ///< Cost of the best parameter vector.
    ///<
  double* NewValues; ///< Temporary new parameter buffer, with real values.
    ///<
  int StallCount; ///< The number of iterations without improvement.
    ///<
  double HiBound; ///< Higher cost bound, for StallCount estimation. May not
    ///< be used by the optimizer.
    ///<
  double AvgCost; ///< Average cost in the latest batch. May not be used by
    ///< the optimizer.
    ///<
  CBiteOptHistBase* Hists[ MaxHistCount ]; ///< Pointers to histogram
    ///< objects, for indexed access in some cases.
    ///<
  const char* HistNames[ MaxHistCount ]; ///< Histogram names.
    ///<
  int HistCount; ///< The number of histograms in use.
    ///<
  static const int MaxApplyHists = 32; /// The maximal number of histograms
    ///< that can be used during the optimize() function call.
    ///<
  CBiteOptHistBase* ApplyHists[ MaxApplyHists ]; ///< Histograms used in
    ///< "selects" during the optimize() function call.
    ///<
  int ApplyHistsCount; ///< The number of "selects" used during the
    ///< optimize() function call.
    ///<

  virtual void initBuffers( const int aParamCount, const int aPopSize )
  {
    CBiteOptParPops< ptype > :: initBuffers( aParamCount, aPopSize );

    MinValues = new double[ ParamCount ];
    MaxValues = new double[ ParamCount ];
    DiffValues = new double[ ParamCount ];
    DiffValuesI = new double[ ParamCount ];
    BestValues = new double[ ParamCount ];
    NewValues = new double[ ParamCount ];
  }

  virtual void deleteBuffers()
  {
    CBiteOptParPops< ptype > :: deleteBuffers();

    delete[] MinValues;
    delete[] MaxValues;
    delete[] DiffValues;
    delete[] DiffValuesI;
    delete[] BestValues;
    delete[] NewValues;
  }

  /**
   * Function resets common variables used by optimizers to their default
   * values, including registered histograms, calls the resetCurPopPos()
   * and updateDiffValues() functions. This function is usually called in
   * the init() function of the optimizer.
   */

  void resetCommonVars( CBiteRnd& rnd )
  {
    updateDiffValues();
    resetCurPopPos();

    CurPopSize = PopSize;
    CurPopSize1 = PopSize - 1;
    BestCost = 1e300;
    StallCount = 0;
    HiBound = 1e300;
    AvgCost = 0.0;
    ApplyHistsCount = 0;

    int i;

    for( i = 0; i < HistCount; i++ )
    {
      Hists[ i ] -> reset( rnd );
    }
  }

  /**
   * Function updates values in the DiffValues array, based on values in the
   * MinValues and MaxValues arrays.
   */

  void updateDiffValues()
  {
    int i;

    if( (ptype) 0.25 == 0 )
    {
      for( i = 0; i < ParamCount; i++ )
      {
        const double d = MaxValues[ i ] - MinValues[ i ];

        DiffValues[ i ] = d / IntMantMult;
        DiffValuesI[ i ] = IntMantMult / d;
      }
    }
    else
    {
      for( i = 0; i < ParamCount; i++ )
      {
        DiffValues[ i ] = MaxValues[ i ] - MinValues[ i ];
        DiffValuesI[ i ] = 1.0 / DiffValues[ i ];
      }
    }
  }

  /**
   * Function updates BestCost value and BestValues array, if the specified
   * NewCost is better.
   *
   * @param NewCost New solution's cost.
   * @param UpdValues New solution's values. The values should be in the
   * "real" value range.
   */

  void updateBestCost( const double NewCost, const double* const UpdValues )
  {
    if( NewCost <= BestCost )
    {
      BestCost = NewCost;

      memcpy( BestValues, UpdValues,
        ParamCount * sizeof( BestValues[ 0 ]));
    }
  }

  /**
   * Function returns specified parameter's value taking into account
   * minimal and maximal value range.
   *
   * @param NormParams Parameter vector of interest, in normalized scale.
   * @param i Parameter index.
   */

  double getRealValue( const ptype* const NormParams, const int i ) const
  {
    return( MinValues[ i ] + DiffValues[ i ] * NormParams[ i ]);
  }

  /**
   * Function wraps the specified parameter value so that it stays in the
   * [MinValue; MaxValue] real range, by wrapping it over the boundaries
   * using random operator. This operation improves convergence in
   * comparison to clamping.
   *
   * @param v Parameter value to wrap.
   * @param i Parameter index.
   * @return Wrapped parameter value.
   */

  double wrapParamReal( CBiteRnd& rnd, const double v, const int i ) const
  {
    const double minv = MinValues[ i ];

    if( v < minv )
    {
      const double dv = DiffValues[ i ];

      if( v > minv - dv )
      {
        return( minv + rnd.getRndValue() * ( minv - v ));
      }

      return( minv + rnd.getRndValue() * dv );
    }

    const double maxv = MaxValues[ i ];

    if( v > maxv )
    {
      const double dv = DiffValues[ i ];

      if( v < maxv + dv )
      {
        return( maxv - rnd.getRndValue() * ( v - maxv ));
      }

      return( maxv - rnd.getRndValue() * dv );
    }

    return( v );
  }

  /**
   * Function adds a histogram to the Hists list.
   *
   * @param h Histogram object to add.
   * @param hname Histogram's name, should be a static constant.
   */

  void addHist( CBiteOptHistBase& h, const char* const hname )
  {
    Hists[ HistCount ] = &h;
    HistNames[ HistCount ] = hname;
    HistCount++;
  }

  /**
   * Function performs choice selection based on the specified histogram,
   * and adds the histogram to apply list.
   *
   * @param Hist Histogram.
   * @param rnd PRNG object.
   */

  template< class T >
  int select( T& Hist, CBiteRnd& rnd )
  {
    ApplyHists[ ApplyHistsCount ] = &Hist;
    ApplyHistsCount++;

    return( Hist.select( rnd ));
  }

  /**
   * Function applies histogram increments on optimization success.
   *
   * @param rnd PRNG object.
   */

  void applyHistsIncr( CBiteRnd& rnd )
  {
    const int c = ApplyHistsCount;
    ApplyHistsCount = 0;

    int i;

    for( i = 0; i < c; i++ )
    {
      ApplyHists[ i ] -> incr( rnd );
    }
  }

  /**
   * Function applies histogram decrements on optimization fail.
   *
   * @param rnd PRNG object.
   */

  void applyHistsDecr( CBiteRnd& rnd )
  {
    const int c = ApplyHistsCount;
    ApplyHistsCount = 0;

    int i;

    for( i = 0; i < c; i++ )
    {
      ApplyHists[ i ] -> decr( rnd );
    }
  }
};

#endif // BITEAUX_INCLUDED

// nmsopt.h
//$ nocpp

/**
 * @file nmsopt.h
 *
 * @brief The inclusion file for the CNMSeqOpt class.
 *
 * @section license License
 *
 * Copyright (c) 2016-2021 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @version 2021.28
 */

#ifndef NMSOPT_INCLUDED
#define NMSOPT_INCLUDED

/**
 * Sequential Nelder-Mead simplex method.
 *
 * Description is available at https://github.com/avaneev/biteopt
 */

class CNMSeqOpt : public CBiteOptBase< double >
{
public:
  CNMSeqOpt()
    : x2( NULL )
  {
  }

  /**
   * Function updates dimensionality of *this object.
   *
   * @param aParamCount The number of parameters being optimized.
   * @param PopSize0 The number of elements in population to use. If set to
   * 0 or negative, the default formula will be used.
   */

  void updateDims( const int aParamCount, const int PopSize0 = 0 )
  {
    const int aPopSize = ( PopSize0 > 0 ? PopSize0 :
      ( aParamCount + 1 ) * 4 );

    if( aParamCount == ParamCount && aPopSize == PopSize )
    {
      return;
    }

    N = aParamCount;
    M = aPopSize;
    M1m = 1.0 / ( M - 1 );

    initBuffers( N, M );
  }

  /**
   * Function initializes *this optimizer. Performs N=PopSize objective
   * function evaluations.
   *
   * @param rnd Random number generator.
   * @param InitParams If not NULL, initial parameter vector, also used as
   * centroid.
   * @param InitRadius Initial radius, relative to the default value. Set
   * to negative to use uniformly-random sampling.
   */

  void init( CBiteRnd& rnd, const double* const InitParams = NULL,
    const double InitRadius = 1.0 )
  {
    getMinValues( MinValues );
    getMaxValues( MaxValues );

    resetCommonVars( rnd );

    // Initialize parameter vectors, costs and centroid.

    double* const xx = x[ 0 ];
    int i;
    int j;

    if( InitParams != NULL )
    {
      memcpy( xx, InitParams, N * sizeof( x[ 0 ]));
    }
    else
    {
      for( i = 0; i < N; i++ )
      {
        xx[ i ] = MinValues[ i ] + DiffValues[ i ] * 0.5;
      }
    }

    xlo = 0;

    if( InitRadius <= 0.0 )
    {
      for( j = 1; j < M; j++ )
      {
        double* const xj = x[ j ];

        for( i = 0; i < N; i++ )
        {
          xj[ i ] = MinValues[ i ] + DiffValues[ i ] *
            rnd.getRndValue();
        }
      }
    }
    else
    {
      const double sd = 0.25 * InitRadius;

      for( j = 1; j < M; j++ )
      {
        double* const xj = x[ j ];

        for( i = 0; i < N; i++ )
        {
          xj[ i ] = xx[ i ] + DiffValues[ i ] *
            getGaussian( rnd ) * sd;
        }
      }
    }

    State = stReflection;
    DoInitEvals = true;
  }

  /**
   * Function performs the parameter optimization iteration that involves 1
   * objective function evaluation.
   *
   * @param rnd Random number generator.
   * @param OutCost If not NULL, pointer to variable that receives cost
   * of the newly-evaluated solution.
   * @param OutValues If not NULL, pointer to array that receives
   * newly-evaluated parameter vector, in real scale.
   * @return The number of non-improving iterations so far.
   */

  int optimize( CBiteRnd& rnd, double* const OutCost = NULL,
    double* const OutValues = NULL )
  {
    int i;

    if( DoInitEvals )
    {
      y[ CurPopPos ] = eval( rnd, x[ CurPopPos ], OutCost, OutValues );

      if( y[ CurPopPos ] < y[ xlo ])
      {
        xlo = CurPopPos;
      }

      CurPopPos++;

      if( CurPopPos == M )
      {
        DoInitEvals = false;
        calccent();
      }

      return( 0 );
    }

    StallCount++;

    static const double alpha = 1.0; // Reflection coeff.
    static const double gamma = 2.0; // Expansion coeff.
    static const double rho = -0.5; // Contraction coeff.
    static const double sigma = 0.5; // Reduction coeff.

    double* const xH = x[ xhi ]; // Highest cost parameter vector.

    switch( State )
    {
      case stReflection:
      {
        for( i = 0; i < N; i++ )
        {
          x1[ i ] = x0[ i ] + alpha * ( x0[ i ] - xH[ i ]);
        }

        y1 = eval( rnd, x1, OutCost, OutValues );

        if( y1 >= y[ xlo ] && y1 < y[ xhi2 ])
        {
          copy( x1, y1 );
          StallCount = 0;
        }
        else
        {
          if( y1 < y[ xlo ])
          {
            State = stExpansion;
            StallCount = 0;
          }
          else
          {
            State = stContraction;
          }
        }

        break;
      }

      case stExpansion:
      {
        for( i = 0; i < N; i++ )
        {
          x2[ i ] = x0[ i ] + gamma * ( x0[ i ] - xH[ i ]);
        }

        const double y2 = eval( rnd, x2, OutCost, OutValues );
        xlo = xhi;

        if( y2 < y1 )
        {
          copy( x2, y2 );
        }
        else
        {
          copy( x1, y1 );
        }

        State = stReflection;
        break;
      }

      case stContraction:
      {
        for( i = 0; i < N; i++ )
        {
          x2[ i ] = x0[ i ] + rho * ( x0[ i ] - xH[ i ]);
        }

        const double y2 = eval( rnd, x2, OutCost, OutValues );

        if( y2 < y[ xhi ])
        {
          if( y2 < y[ xlo ])
          {
            xlo = xhi;
          }

          copy( x2, y2 );
          State = stReflection;
          StallCount = 0;
        }
        else
        {
          rx = x[ xlo ];
          rj = 0;
          State = stReduction;
        }

        break;
      }

      case stReduction:
      {
        double* xx = x[ rj ];

        if( xx == rx )
        {
          rj++;
          xx = x[ rj ];
        }

        for( i = 0; i < N; i++ )
        {
          xx[ i ] = rx[ i ] + sigma * ( xx[ i ] - rx[ i ]);
        }

        y[ rj ] = eval( rnd, xx, OutCost, OutValues );

        if( y[ rj ] < y[ xlo ])
        {
          xlo = rj;
          StallCount = 0;
        }

        rj++;

        if( rj == M || ( rj == M - 1 && x[ rj ] == rx ))
        {
          calccent();
          State = stReflection;
        }

        break;
      }
    }

    return( StallCount );
  }

private:
  int N; ///< The total number of internal parameter values in use.
    ///<
  int M; ///< The number of points in a simplex.
    ///<
  double M1m; ///< = 1 / ( M - 1 ).
    ///<
  int xlo; ///< Current lowest cost parameter vector.
    ///<
  int xhi; ///< Current highest cost parameter vector.
    ///<
  int xhi2; ///< Current second highest cost parameter vector.
    ///<
  double** x; ///< Parameter vectors for all points.
    ///<
  double* y; ///< Parameter vector costs.
    ///<
  double* x0; // Centroid parameter vector.
    ///<
  double* x1; ///< Temporary parameter vector 1.
    ///<
  double y1; ///< Cost of temporary parameter vector 1.
    ///<
  double* x2; ///< Temporary parameter vector 2.
    ///<
  double* rx; ///< Lowest cost parameter vector used during reduction.
    ///<
  int rj; ///< Current vector during reduction.
    ///<
  bool DoInitEvals; ///< "True" if initial evaluations should be performed.
    ///<

  /**
   * Algorithm's state automata states.
   */

  enum EState
  {
    stReflection, // Reflection.
    stExpansion, // Expansion.
    stContraction, // Contraction.
    stReduction // Reduction.
  };

  EState State; ///< Current optimization state.
    ///<

  virtual void initBuffers( const int aParamCount, const int aPopSize )
  {
    CBiteOptBase :: initBuffers( aParamCount, aPopSize );

    x = PopParams;
    y = PopCosts;
    x0 = CentParams;
    x1 = TmpParams;
    x2 = new double[ N ];
  }

  virtual void deleteBuffers()
  {
    CBiteOptBase :: deleteBuffers();

    delete[] x2;
  }

  /**
   * Function finds the highest-cost vector.
   */

  void findhi()
  {
    if( y[ 0 ] > y[ 1 ])
    {
      xhi = 0;
      xhi2 = 1;
    }
    else
    {
      xhi = 1;
      xhi2 = 0;
    }

    int j;

    for( j = 2; j < M; j++ )
    {
      if( y[ j ] > y[ xhi ])
      {
        xhi2 = xhi;
        xhi = j;
      }
      else
      if( y[ j ] > y[ xhi2 ])
      {
        xhi2 = j;
      }
    }
  }

  /**
   * Function calculates the centroid vector.
   */

  void calccent()
  {
    findhi();
    int i;

    for( i = 0; i < N; i++ )
    {
      x0[ i ] = 0.0;
    }

    int j;

    for( j = 0; j < M; j++ )
    {
      if( j != xhi )
      {
        const double* const xx = x[ j ];

        for( i = 0; i < N; i++ )
        {
          x0[ i ] += xx[ i ];
        }
      }
    }

    for( i = 0; i < N; i++ )
    {
      x0[ i ] *= M1m;
    }
  }

  /**
   * Function replaces the highest-cost vector with a new vector.
   *
   * @param ip Input vector.
   * @param cost Input vector's cost.
   */

  void copy( const double* const ip, const double cost )
  {
    y[ xhi ] = cost;
    double* const xH = x[ xhi ];
    int i;

    memcpy( xH, ip, N * sizeof( xH[ 0 ]));
    findhi();

    const double* const nxH = x[ xhi ];

    if( xH != nxH )
    {
      for( i = 0; i < N; i++ )
      {
        x0[ i ] += ( xH[ i ] - nxH[ i ]) * M1m;
      }
    }
  }

  /**
   * Function evaluates parameter vector and applies value range wrapping,
   * also records a new best solution.
   *
   * @param rnd Random number generator.
   * @param p Parameter vector to evaluate.
   * @param OutCost If not NULL, pointer to variable that receives cost
   * of the newly-evaluated solution.
   * @param OutValues If not NULL, pointer to array that receives
   * newly-evaluated parameter vector, in real scale.
   */

  double eval( CBiteRnd& rnd, const double* p,
    double* const OutCost = NULL, double* const OutValues = NULL )
  {
    int i;

    for( i = 0; i < N; i++ )
    {
      NewValues[ i ] = wrapParamReal( rnd, p[ i ], i );
    }

    const double cost = optcost( NewValues );

    if( OutCost != NULL )
    {
      *OutCost = cost;
    }

    if( OutValues != NULL )
    {
      memcpy( OutValues, NewValues, N * sizeof( OutValues[ 0 ]));
    }

    updateBestCost( cost, NewValues );

    return( cost );
  }
};

#endif // NMSOPT_INCLUDED

//$ nocpp

/**
 * @file spheropt.h
 *
 * @brief The inclusion file for the CSpherOpt class.
 *
 * @section license License
 *
 * Copyright (c) 2016-2021 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @version 2021.17
 */

#ifndef SPHEROPT_INCLUDED
#define SPHEROPT_INCLUDED

/**
 * "Converging hyper-spheroid" optimizer class. Simple, converges quite fast.
 *
 * Description is available at https://github.com/avaneev/biteopt
 */

class CSpherOpt : public CBiteOptBase< double >
{
public:
  double Jitter; ///< Solution sampling random jitter, improves convergence
    ///< at low dimensions. Usually, a fixed value.
    ///<

  CSpherOpt()
    : WPopCent( NULL )
    , WPopRad( NULL )
  {
    Jitter = 2.5;

    addHist( CentPowHist, "CentPowHist" );
    addHist( RadPowHist, "RadPowHist" );
    addHist( EvalFacHist, "EvalFacHist" );
  }

  /**
   * Function updates dimensionality of *this object.
   *
   * @param aParamCount The number of parameters being optimized.
   * @param PopSize0 The number of elements in population to use. If set to
   * 0 or negative, the default formula will be used.
   */

  void updateDims( const int aParamCount, const int PopSize0 = 0 )
  {
    const int aPopSize = ( PopSize0 > 0 ? PopSize0 : 14 + aParamCount );

    if( aParamCount == ParamCount && aPopSize == PopSize )
    {
      return;
    }

    initBuffers( aParamCount, aPopSize );

    JitMult = 2.0 * Jitter / aParamCount;
    JitOffs = 1.0 - JitMult * 0.5;
  }

  /**
   * Function initializes *this optimizer.
   *
   * @param rnd Random number generator.
   * @param InitParams If not NULL, initial parameter vector, also used as
   * centroid.
   * @param InitRadius Initial radius, multiplier relative to the default
   * sigma value.
   */

  void init( CBiteRnd& rnd, const double* const InitParams = NULL,
    const double InitRadius = 1.0 )
  {
    getMinValues( MinValues );
    getMaxValues( MaxValues );

    resetCommonVars( rnd );

    Radius = 0.5 * InitRadius;
    EvalFac = 2.0;
    cure = 0;
    curem = (int) ceil( CurPopSize * EvalFac );

    // Provide initial centroid and sigma.

    int i;

    if( InitParams == NULL )
    {
      for( i = 0; i < ParamCount; i++ )
      {
        CentParams[ i ] = 0.5;
      }

      DoCentEval = false;
    }
    else
    {
      for( i = 0; i < ParamCount; i++ )
      {
        CentParams[ i ] = wrapParam( rnd,
          ( InitParams[ i ] - MinValues[ i ]) / DiffValues[ i ]);
      }

      DoCentEval = true;
    }
  }

  /**
   * Function performs the parameter optimization iteration that involves 1
   * objective function evaluation.
   *
   * @param rnd Random number generator.
   * @param OutCost If not NULL, pointer to variable that receives cost
   * of the newly-evaluated solution.
   * @param OutValues If not NULL, pointer to array that receives a
   * newly-evaluated parameter vector, in real scale, in real value bounds.
   * @return The number of non-improving iterations so far.
   */

  int optimize( CBiteRnd& rnd, double* const OutCost = NULL,
    double* const OutValues = NULL )
  {
    double* const Params = PopParams[ CurPopPos ];
    int i;

    if( DoCentEval )
    {
      DoCentEval = false;

      for( i = 0; i < ParamCount; i++ )
      {
        Params[ i ] = CentParams[ i ];
        NewValues[ i ] = getRealValue( CentParams, i );
      }
    }
    else
    {
      double s2 = 1e-300;

      for( i = 0; i < ParamCount; i++ )
      {
        Params[ i ] = rnd.getRndValue() - 0.5;
        s2 += Params[ i ] * Params[ i ];
      }

      const double d = Radius / sqrt( s2 );

      if( ParamCount > 4 )
      {
        for( i = 0; i < ParamCount; i++ )
        {
          Params[ i ] = wrapParam( rnd,
            CentParams[ i ] + Params[ i ] * d );

          NewValues[ i ] = getRealValue( Params, i );
        }
      }
      else
      {
        for( i = 0; i < ParamCount; i++ )
        {
          const double m = JitOffs + rnd.getRndValue() * JitMult;

          Params[ i ] = wrapParam( rnd,
            CentParams[ i ] + Params[ i ] * d * m );

          NewValues[ i ] = getRealValue( Params, i );
        }
      }
    }

    const double NewCost = optcost( NewValues );

    if( OutCost != NULL )
    {
      *OutCost = NewCost;
    }

    if( OutValues != NULL )
    {
      memcpy( OutValues, NewValues,
        ParamCount * sizeof( OutValues[ 0 ]));
    }

    updateBestCost( NewCost, NewValues );

    if( CurPopPos < CurPopSize )
    {
      sortPop( NewCost, CurPopPos );
      CurPopPos++;
    }
    else
    {
      if( isAcceptedCost( NewCost ))
      {
        memcpy( PopParams[ CurPopSize1 ], Params,
          ParamCount * sizeof( PopParams[ 0 ][ 0 ]));

        sortPop( NewCost, CurPopSize1 );
      }
    }

    AvgCost += NewCost;
    cure++;

    if( cure >= curem )
    {
      AvgCost /= cure;

      if( AvgCost < HiBound )
      {
        HiBound = AvgCost;
        StallCount = 0;

        applyHistsIncr( rnd );
      }
      else
      {
        StallCount += cure;

        applyHistsDecr( rnd );
      }

      AvgCost = 0.0;
      CurPopPos = 0;
      cure = 0;

      update( rnd );

      curem = (int) ceil( CurPopSize * EvalFac );
    }

    return( StallCount );
  }

protected:
  double* WPopCent; ///< Weighting coefficients for centroid.
    ///<
  double* WPopRad; ///< Weighting coefficients for radius.
    ///<
  double JitMult; ///< Jitter multiplier.
    ///<
  double JitOffs; ///< Jitter multiplier offset.
    ///<
  double Radius; ///< Current radius.
    ///<
  double EvalFac; ///< Evaluations factor.
    ///<
  int cure; ///< Current evaluation index.
    ///<
  int curem; ///< "cure" value threshold.
    ///<
  bool DoCentEval; ///< "True" if an initial objective function evaluation
    ///< at centroid point is required.
    ///<
  CBiteOptHist< 4 > CentPowHist; ///< Centroid power factor histogram.
    ///<
  CBiteOptHist< 4 > RadPowHist; ///< Radius power factor histogram.
    ///<
  CBiteOptHist< 3 > EvalFacHist; ///< EvalFac histogram.
    ///<

  virtual void initBuffers( const int aParamCount, const int aPopSize )
  {
    CBiteOptBase< double > :: initBuffers( aParamCount, aPopSize );

    WPopCent = new double[ aPopSize ];
    WPopRad = new double[ aPopSize ];
  }

  virtual void deleteBuffers()
  {
    CBiteOptBase< double > :: deleteBuffers();

    delete[] WPopCent;
    delete[] WPopRad;
  }

  /**
   * Function updates centroid and radius.
   *
   * @param rnd PRNG object.
   */

  void update( CBiteRnd& rnd )
  {
    static const double WCent[ 4 ] = { 4.5, 6.0, 7.5, 10.0 };
    static const double WRad[ 4 ] = { 14.0, 16.0, 18.0, 20.0 };
    static const double EvalFacs[ 3 ] = { 2.1, 2.0, 1.9 };

    const double CentFac = WCent[ select( CentPowHist, rnd )];
    const double RadFac = WRad[ select( RadPowHist, rnd )];
    EvalFac = EvalFacs[ select( EvalFacHist, rnd )];

    const double lm = 1.0 / curem;
    double s1 = 0.0;
    double s2 = 0.0;
    int i;

    for( i = 0; i < CurPopSize; i++ )
    {
      const double l = 1.0 - i * lm;

      const double v1 = pow( l, CentFac );
      WPopCent[ i ] = v1;
      s1 += v1;

      const double v2 = pow( l, RadFac );
      WPopRad[ i ] = v2;
      s2 += v2;
    }

    s1 = 1.0 / s1;
    s2 = 1.0 / s2;

    const double* ip = PopParams[ 0 ];
    double* const cp = CentParams;
    const double* const wc = WPopCent;
    double w = wc[ 0 ] * s1;

    for( i = 0; i < ParamCount; i++ )
    {
      cp[ i ] = ip[ i ] * w;
    }

    int j;

    for( j = 1; j < CurPopSize; j++ )
    {
      ip = PopParams[ j ];
      w = wc[ j ] * s1;

      for( i = 0; i < ParamCount; i++ )
      {
        cp[ i ] += ip[ i ] * w;
      }
    }

    const double* const rc = WPopRad;
    Radius = 0.0;

    for( j = 0; j < CurPopSize; j++ )
    {
      ip = PopParams[ j ];
      double s = 0.0;

      for( i = 0; i < ParamCount; i++ )
      {
        const double d = ip[ i ] - cp[ i ];
        s += d * d;
      }

      Radius += s * rc[ j ] * s2;
    }

    Radius = sqrt( Radius );
  }
};

#endif // SPHEROPT_INCLUDED

//$ nocpp

/**
 * @file biteopt.h
 *
 * @brief The inclusion file for the CBiteOpt and CBiteOptDeep classes.
 *
 * @section license License
 *
 * Copyright (c) 2016-2021 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @version 2021.28.1
 */

#ifndef BITEOPT_INCLUDED
#define BITEOPT_INCLUDED


/**
 * BiteOpt optimization class. Implements a stochastic non-linear
 * bound-constrained derivative-free optimization method.
 *
 * Description is available at https://github.com/avaneev/biteopt
 */

class CBiteOpt : public CBiteOptBase< int64_t >
{
public:
  typedef int64_t ptype; ///< Parameter value storage type (should be a
    ///< signed integer type, same as CBiteOptBase template parameter).
    ///<

  CBiteOpt()
  {
    setParPopCount( 4 );

    addHist( MethodHist, "MethodHist" );
    addHist( M1Hist, "M1Hist" );
    addHist( M1AHist, "M1AHist" );
    addHist( M1BHist, "M1BHist" );
    addHist( M1BAHist, "M1BAHist" );
    addHist( M1BBHist, "M1BBHist" );
    addHist( PopChangeHist, "PopChangeHist" );
    addHist( ParOpt2Hist, "ParOpt2Hist" );
    addHist( ParPopPHist[ 0 ], "ParPopPHist[ 0 ]" );
    addHist( ParPopPHist[ 1 ], "ParPopPHist[ 1 ]" );
    addHist( ParPopPHist[ 2 ], "ParPopPHist[ 2 ]" );
    addHist( ParPopHist[ 0 ], "ParPopHist[ 0 ]" );
    addHist( ParPopHist[ 1 ], "ParPopHist[ 1 ]" );
    addHist( ParPopHist[ 2 ], "ParPopHist[ 2 ]" );
    addHist( AltPopPHist, "AltPopPHist" );
    addHist( AltPopHist[ 0 ], "AltPopHist[ 0 ]" );
    addHist( AltPopHist[ 1 ], "AltPopHist[ 1 ]" );
    addHist( AltPopHist[ 2 ], "AltPopHist[ 2 ]" );
    addHist( MinSolPwrHist[ 0 ], "MinSolPwrHist[ 0 ]" );
    addHist( MinSolPwrHist[ 1 ], "MinSolPwrHist[ 1 ]" );
    addHist( MinSolPwrHist[ 2 ], "MinSolPwrHist[ 2 ]" );
    addHist( MinSolMulHist[ 0 ], "MinSolMulHist[ 0 ]" );
    addHist( MinSolMulHist[ 1 ], "MinSolMulHist[ 1 ]" );
    addHist( MinSolMulHist[ 2 ], "MinSolMulHist[ 2 ]" );
    addHist( Gen1AllpHist, "Gen1AllpHist" );
    addHist( Gen1MoveHist, "Gen1MoveHist" );
    addHist( Gen1MoveAsyncHist, "Gen1MoveAsyncHist" );
    addHist( Gen1MoveDEHist, "Gen1MoveDEHist" );
    addHist( Gen1MoveSpanHist, "Gen1MoveSpanHist" );
    addHist( Gen4MixFacHist, "Gen4MixFacHist" );
    addHist( Gen5BinvHist, "Gen5BinvHist" );
    addHist( *ParOpt.getHists()[ 0 ], "ParOpt.CentPowHist" );
    addHist( *ParOpt.getHists()[ 1 ], "ParOpt.RadPowHist" );
    addHist( *ParOpt.getHists()[ 2 ], "ParOpt.EvalFacHist" );
  }

  /**
   * Function updates dimensionality of *this object. Function does nothing
   * if dimensionality has not changed since the last call. This function
   * should be called at least once before calling the init() function.
   *
   * @param aParamCount The number of parameters being optimized.
   * @param PopSize0 The number of elements in population to use. If set to
   * 0 or negative, the default formula will be used.
   */

  void updateDims( const int aParamCount, const int PopSize0 = 0 )
  {
    const int aPopSize = ( PopSize0 > 0 ? PopSize0 :
      13 + aParamCount * 3 );

    if( aParamCount == ParamCount && aPopSize == PopSize )
    {
      return;
    }

    initBuffers( aParamCount, aPopSize );

    ParOpt.Owner = this;
    ParOpt.updateDims( aParamCount );
    ParOptPop.initBuffers( aParamCount, aPopSize );

    ParOpt2.Owner = this;
    ParOpt2.updateDims( aParamCount );
    ParOpt2Pop.initBuffers( aParamCount, aPopSize );
  }

  /**
   * Function initializes *this optimizer. Does not perform objective
   * function evaluations.
   *
   * @param rnd Random number generator.
   * @param InitParams If not NULL, initial parameter vector, also used as
   * centroid for initial population vectors.
   * @param InitRadius Initial radius, multiplier relative to the default
   * sigma value.
   */

  void init( CBiteRnd& rnd, const double* const InitParams = NULL,
    const double InitRadius = 1.0 )
  {
    getMinValues( MinValues );
    getMaxValues( MaxValues );

    resetCommonVars( rnd );

    // Initialize solution vectors randomly, calculate objective function
    // values of these solutions.

    const double sd = 0.25 * InitRadius;
    int i;
    int j;

    if( InitParams == NULL )
    {
      for( j = 0; j < PopSize; j++ )
      {
        ptype* const p = PopParams[ j ];

        for( i = 0; i < ParamCount; i++ )
        {
          p[ i ] = wrapParam( rnd, getGaussianInt(
            rnd, sd, IntMantMult >> 1 ));
        }
      }
    }
    else
    {
      ptype* const p0 = PopParams[ 0 ];

      for( i = 0; i < ParamCount; i++ )
      {
        p0[ i ] = wrapParam( rnd,
          (ptype) (( InitParams[ i ] - MinValues[ i ]) /
          DiffValues[ i ]));
      }

      for( j = 1; j < PopSize; j++ )
      {
        ptype* const p = PopParams[ j ];

        for( i = 0; i < ParamCount; i++ )
        {
          p[ i ] = wrapParam( rnd,
            getGaussianInt( rnd, sd, p0[ i ]));
        }
      }
    }

    updateCentroid();

    AllpProbDamp = 1.8 / ParamCount;
    CentUpdateCtr = 0;

    ParOpt.init( rnd, InitParams, InitRadius );
    ParOpt2.init( rnd, InitParams, InitRadius );
    UseParOpt = 0;

    ParOptPop.resetCurPopPos();
    ParOpt2Pop.resetCurPopPos();

    DoInitEvals = true;
  }

  /**
   * Function performs the parameter optimization iteration that involves 1
   * objective function evaluation.
   *
   * @param rnd Random number generator.
   * @param PushOpt Optimizer where the recently obtained solution should be
   * "pushed", used for deep optimization algorithm.
   * @return The number of non-improving iterations so far. A high value
   * means optimizer has reached an optimization plateau. The suggested
   * threshold value is ParamCount * 64. When this value was reached, the
   * probability of plateau is high. This value, however, should not be
   * solely relied upon when considering a stopping criteria: a hard
   * iteration limit should be always used as in some cases convergence time
   * may be very high with small, but frequent improving steps. This value
   * is best used to allocate iteration budget between optimization attempts
   * more efficiently.
   */

  int optimize(CBiteRnd &rnd, CBiteOpt *const PushOpt = NULL) {
    int i;

    if (DoInitEvals) {
      const ptype *const p = PopParams[CurPopPos];

      for (i = 0; i < ParamCount; i++) {
        NewValues[i] = getRealValue(p, i);
      }

      const double NewCost = optcost( NewValues );
      sortPop( NewCost, CurPopPos );
      updateBestCost( NewCost, NewValues );

      CurPopPos++;

      if (CurPopPos == PopSize) {
        for (i = 0; i < ParPopCount; i++) {
          ParPops[i]->copy(*this);
        }

        DoInitEvals = false;
      }

      return( 0 );
    }

    bool DoEval = true;
    double NewCost;
    const int SelMethod = select( MethodHist, rnd );

    if (SelMethod == 0) {
      generateSol2(rnd);
    } else if (SelMethod == 1) {
      if (select(M1Hist, rnd)) {
        if (select(M1AHist, rnd)) {
          generateSol2b(rnd);
        } else {
          generateSol3(rnd);
        }
      } else {
        const int SelM1B = select(M1BHist, rnd);

        if (SelM1B == 0) {
          if (select(M1BAHist, rnd)) {
            generateSol4(rnd);
          } else {
            generateSol4b(rnd);
          }
        } else if (SelM1B == 1) {
          if (select(M1BBHist, rnd)) {
            generateSol5(rnd);
          } else {
            generateSol5b(rnd);
          }
        } else {
          generateSol6(rnd);
        }
      }
    } else if (SelMethod == 2) {
      generateSol1(rnd);
    } else {
      DoEval = false;
      CBiteOptPop *UpdPop;

      if (UseParOpt == 1) {
        // Re-assign optimizer 2 based on comparison of its
        // efficiency with optimizer 1.

        UseParOpt = select( ParOpt2Hist, rnd );
      }

      if (UseParOpt == 0) {
        const int sc = ParOpt.optimize(rnd, &NewCost, NewValues);

        if (sc > 0) {
          UseParOpt = 1; // On stall, select optimizer 2.
        }

        if (sc > ParamCount * 64) {
          ParOpt.init(rnd, getBestParams());
          ParOptPop.resetCurPopPos();
        }

        UpdPop = &ParOptPop;
      } else {
        const int sc = ParOpt2.optimize( rnd, &NewCost, NewValues );

        if (sc > 0) {
          UseParOpt = 0; // On stall, select optimizer 1.
        }

        if (sc > ParamCount * 16) {
          ParOpt2.init(rnd, getBestParams(), -1.0);
          ParOpt2Pop.resetCurPopPos();
        }

        UpdPop = &ParOpt2Pop;
      }

      for (i = 0; i < ParamCount; i++) {
        TmpParams[i] = (ptype)((NewValues[i] - MinValues[i]) * DiffValuesI[i]);
      }

      UpdPop -> updatePop( NewCost, TmpParams, false, true );
    }

    if (DoEval) {
      // Evaluate objective function with new parameters, if the
      // solution was not provided by the parallel optimizer.
      // Wrap parameter values so that they stay in the [0; 1] range.

      for (i = 0; i < ParamCount; i++) {
        TmpParams[i] = wrapParam(rnd, TmpParams[i]);
        NewValues[i] = getRealValue(TmpParams, i);
      }

      NewCost = optcost( NewValues );
    }

    updateBestCost( NewCost, NewValues );

    if (!isAcceptedCost(NewCost)) {
      // Upper bound cost constraint check failed, reject this solution.

      applyHistsDecr( rnd );

      StallCount++;

      if (CurPopSize < PopSize) {
        if (select(PopChangeHist, rnd) == 0) {
          // Increase population size on fail.

          incrCurPopSize( CurPopSize1 -
            (int) ( rnd.getRndValueSqr() * CurPopSize ));
        }
      }
    } else {
      applyHistsIncr( rnd );

      if (NewCost == PopCosts[CurPopSize1]) {
        StallCount++;
      } else {
        StallCount = 0;
      }

      updatePop( NewCost, TmpParams, false, false );

      if (PushOpt != NULL && PushOpt != this && !PushOpt->DoInitEvals) {
        PushOpt->updatePop(NewCost, TmpParams, false, true);
        PushOpt->updateParPop(NewCost, TmpParams);
      }

      if (CurPopSize > PopSize / 2) {
        if (select(PopChangeHist, rnd) == 1) {
          // Decrease population size on success.

          decrCurPopSize();
        }
      }
    }

    // "Diverging populations" technique.

    updateParPop( NewCost, TmpParams );

    CentUpdateCtr++;

    if (CentUpdateCtr >= CurPopSize * 32) {
      // Update centroids of parallel populations that use running
      // average, to reduce error accumulation.

      CentUpdateCtr = 0;

      for (i = 0; i < ParPopCount; i++) {
        ParPops[i]->updateCentroid();
      }
    }

    return( StallCount );
  }

protected:
  double AllpProbDamp; ///< Damped Allp probability. Applied for higher
    ///< dimensions as the "all parameter" randomization is ineffective in
    ///< the higher dimensions.
    ///<
  CBiteOptHist< 4 > MethodHist; ///< Population generator 4-method
    ///< histogram.
    ///<
  CBiteOptHist< 2 > M1Hist; ///< Method 1's sub-method histogram.
    ///<
  CBiteOptHist< 2 > M1AHist; ///< Method 1's sub-sub-method A histogram.
    ///<
  CBiteOptHist< 3 > M1BHist; ///< Method 1's sub-sub-method B histogram.
    ///<
  CBiteOptHist< 2 > M1BAHist; ///< Method 1's sub-sub-method A2 histogram.
    ///<
  CBiteOptHist< 2 > M1BBHist; ///< Method 1's sub-sub-method B2 histogram.
    ///<
  CBiteOptHist< 2 > PopChangeHist; ///< Population size change
    ///< histogram.
    ///<
  CBiteOptHist< 2 > ParOpt2Hist; ///< Parallel optimizer 2 use
    ///< histogram.
    ///<
  CBiteOptHist< 2 > ParPopPHist[ 3 ]; ///< Parallel population use
    ///< probability histogram.
    ///<
  CBiteOptHist< 4 > ParPopHist[ 3 ]; ///< Parallel population
    ///< histograms for solution generators (template's Count parameter
    ///< should match ParPopCount).
    ///<
  CBiteOptHist< 2 > AltPopPHist; ///< Alternative population use
    ///< histogram.
    ///<
  CBiteOptHist< 2 > AltPopHist[ 3 ]; ///< Alternative population type use
    ///< histogram.
    ///<
  CBiteOptHist< 4 > MinSolPwrHist[ 3 ]; ///< Index of least-cost
    ///< population, power factor.
    ///<
  CBiteOptHist< 4 > MinSolMulHist[ 3 ]; ///< Index of least-cost
    ///< population, multiplier.
    ///<
  CBiteOptHist< 2 > Gen1AllpHist; ///< Generator method 1's Allp
    ///< histogram.
    ///<
  CBiteOptHist< 2 > Gen1MoveHist; ///< Generator method 1's Move
    ///< histogram.
    ////<
  CBiteOptHist< 2 > Gen1MoveAsyncHist; ///< Generator method 1's Move
    ///< async histogram.
    ///<
  CBiteOptHist< 2 > Gen1MoveDEHist; ///< Generator method 1's Move DE
    ///< histogram.
    ///<
  CBiteOptHist< 4 > Gen1MoveSpanHist; ///< Generator method 1's Move span
    ///< histogram.
    ///<
  CBiteOptHist< 4 > Gen4MixFacHist; ///< Generator method 4's mixing
    ///< count histogram.
    ///<
  CBiteOptHist< 2 > Gen5BinvHist; ///< Generator method 5's random
    ///< inversion technique histogram.
    ///<
  int CentUpdateCtr; ///< Centroid update counter.
    ///<
  bool DoInitEvals; ///< "True" if initial evaluations should be performed.
    ///<

  /**
   * Parallel optimizer class.
   */

  template< class T >
  class CParOpt : public T
  {
  public:
    CBiteOpt* Owner; ///< Owner object.

    virtual void getMinValues( double* const p ) const
    {
      Owner -> getMinValues( p );
    }

    virtual void getMaxValues( double* const p ) const
    {
      Owner -> getMaxValues( p );
    }

    virtual double optcost( const double* const p )
    {
      return( Owner -> optcost( p ));
    }
  };

  CParOpt< CSpherOpt > ParOpt; ///< Parallel optimizer.
    ///<
  CBiteOptPop ParOptPop; ///< Population of parallel optimizer's solutions.
    ///< Includes only its solutions.
    ///<
  CParOpt< CNMSeqOpt > ParOpt2; ///< Parallel optimizer2.
    ///<
  CBiteOptPop ParOpt2Pop; ///< Population of parallel optimizer 2's
    ///< solutions. Includes only its solutions.
    ///<
  int UseParOpt; ///< Parallel optimizer currently being in use.
    ///<

  /**
   * Function updates an appropriate parallel population.
   *
   * @param NewCost Cost of the new solution.
   * @param UpdParams New parameter values.
   */

  void updateParPop( const double NewCost, const ptype* const UpdParams )
  {
    const int p = getMinDistParPop( NewCost, UpdParams );

    if( p >= 0 )
    {
      ParPops[ p ] -> updatePop( NewCost, UpdParams, true, true );
    }
  }

  /**
   * Function selects a parallel population to use for solution generation.
   * With certain probability, *this object's own population will be
   * returned instead of parallel population.
   *
   * @param gi Solution generator index (0-2).
   * @param rnd PRNG object.
   */

  CBiteOptPop& selectParPop( const int gi, CBiteRnd& rnd )
  {
    if( select( ParPopPHist[ gi ], rnd ))
    {
      return( *ParPops[ select( ParPopHist[ gi ], rnd )]);
    }

    return( *this );
  }

  /**
   * Function selects an alternative, parallel optimizer's, population, to
   * use in some solution generators.
   *
   * @param gi Solution generator index (0-2).
   * @param rnd PRNG object.
   */

  CBiteOptPop& selectAltPop( const int gi, CBiteRnd& rnd )
  {
    if( select( AltPopPHist, rnd ))
    {
      if( select( AltPopHist[ gi ], rnd ))
      {
        if( ParOptPop.getCurPopPos() >= CurPopSize )
        {
          return( ParOptPop );
        }
      }
      else
      {
        if( ParOpt2Pop.getCurPopPos() >= CurPopSize )
        {
          return( ParOpt2Pop );
        }
      }
    }

    return( *this );
  }

  /**
   * Function returns a dynamically-selected minimal population index, used
   * in some solution generation methods.
   *
   * @param gi Solution generator index (0-2).
   * @param rnd PRNG object.
   * @param ps Population size.
   */

  int getMinSolIndex( const int gi, CBiteRnd& rnd, const int ps )
  {
    static const double pp[ 4 ] = { 0.05, 0.125, 0.25, 0.5 };
    const double r = ps * pow( rnd.getRndValue(),
      ps * pp[ select( MinSolPwrHist[ gi ], rnd )]);

    static const double rm[ 4 ] = { 0.0, 0.125, 0.25, 0.5 };

    return( (int) ( r * rm[ select( MinSolMulHist[ gi ], rnd )]));
  }

  /**
   * The original "bitmask inversion with random move" solution generator.
   * Most of the time adjusts only a single parameter of a better solution,
   * yet manages to produce excellent "reference points".
   */

  void generateSol1( CBiteRnd& rnd )
  {
    ptype* const Params = TmpParams;

    const CBiteOptPop& ParPop = selectParPop( 0, rnd );

    memcpy( Params, ParPop.getParamsOrdered(
      getMinSolIndex( 0, rnd, ParPop.getCurPopSize() )),
      ParamCount * sizeof( Params[ 0 ]));

    // Select a single random parameter or all parameters for further
    // operations.

    int i;
    int a;
    int b;
    bool DoAllp = false;

    if( rnd.getRndValue() < AllpProbDamp )
    {
      if( select( Gen1AllpHist, rnd ))
      {
        DoAllp = true;
      }
    }

    if( DoAllp )
    {
      a = 0;
      b = ParamCount;
    }
    else
    {
      a = (int) ( rnd.getRndValue() * ParamCount );
      b = a + 1;
    }

    // Bitmask inversion operation, works as the main "driver" of
    // optimization process.

    const double r1 = rnd.getRndValue();
    const double r12 = r1 * r1;
    const int ims = (int) ( r12 * r12 * 48.0 );
    const ptype imask = (ptype) ( ims > 63 ? 0 : IntMantMask >> ims );

    const int im2s = (int) ( rnd.getRndValueSqr() * 96.0 );
    const ptype imask2 = (ptype) ( im2s > 63 ? 0 : IntMantMask >> im2s );

    const int si1 = (int) ( r1 * r12 * CurPopSize );
    const ptype* const rp1 = getParamsOrdered( si1 );

    for( i = a; i < b; i++ )
    {
      Params[ i ] = (( Params[ i ] ^ imask ) +
        ( rp1[ i ] ^ imask2 )) >> 1;
    }

    if( select( Gen1MoveHist, rnd ))
    {
      const int si2 = (int) ( rnd.getRndValueSqr() * CurPopSize );
      const ptype* const rp2 = getParamsOrdered( si2 );

      if( select( Gen1MoveDEHist, rnd ))
      {
        // Apply a DE-based move.

        const ptype* const rp3 = getParamsOrdered(
          CurPopSize1 - si2 );

        for( i = 0; i < ParamCount; i++ )
        {
          Params[ i ] -= ( rp3[ i ] - rp2[ i ]) >> 1;
        }
      }
      else
      {
        if( select( Gen1MoveAsyncHist, rnd ))
        {
          a = 0;
          b = ParamCount;
        }

        // Random move around a random previous solution vector.

        static const double SpanMults[ 4 ] = { 0.5, 1.5, 2.0, 2.5 };

        const double m = SpanMults[ select( Gen1MoveSpanHist, rnd )];
        const double m1 = rnd.getTPDF() * m;
        const double m2 = rnd.getTPDF() * m;

        for( i = a; i < b; i++ )
        {
          Params[ i ] -= (ptype) (( Params[ i ] - rp2[ i ]) * m1 );
          Params[ i ] -= (ptype) (( Params[ i ] - rp2[ i ]) * m2 );
        }
      }
    }
  }

  /**
   * The "Digital Evolution"-based solution generator.
   */

  void generateSol2( CBiteRnd& rnd )
  {
    ptype* const Params = TmpParams;

    const int si1 = getMinSolIndex( 1, rnd, CurPopSize );
    const ptype* const rp1 = getParamsOrdered( si1 );
    const ptype* const rp3 = getParamsOrdered( CurPopSize1 - si1 );

    const int si2 = 1 + (int) ( rnd.getRndValue() * CurPopSize1 );
    const ptype* const rp2 = getParamsOrdered( si2 );

    const int si4 = (int) ( rnd.getRndValueSqr() * CurPopSize );
    const ptype* const rp4 = getParamsOrdered( si4 );
    const ptype* const rp5 = getParamsOrdered( CurPopSize1 - si4 );

    // The "step in the right direction" (Differential Evolution
    // "mutation") operation towards the best (minimal) and away from
    // the worst (maximal) parameter vector, plus a difference of two
    // random vectors.

    int i;

    for( i = 0; i < ParamCount; i++ )
    {
      Params[ i ] = rp1[ i ] - ((( rp3[ i ] - rp2[ i ]) +
        ( rp5[ i ] - rp4[ i ])) >> 1 );
    }
  }

  /**
   * An alternative "Digital Evolution"-based solution generator.
   */

  void generateSol2b( CBiteRnd& rnd )
  {
    ptype* const Params = TmpParams;

    // Select worst and a random previous solution from the ordered list,
    // apply offsets to reduce sensitivity to noise.

    const int si1 = getMinSolIndex( 1, rnd, CurPopSize );
    const ptype* const rp1 = getParamsOrdered( si1 );

    const int si2 = (int) ( rnd.getRndValueSqr() * CurPopSize );
    const ptype* const rp2 = getParamsOrdered( si2 );

    const ptype* const rp3 = getParamsOrdered( CurPopSize1 - si2 );

    // Select two more previous solutions to be used in the mix.

    const CBiteOptPop& AltPop = selectAltPop( 0, rnd );

    const int si4 = (int) ( rnd.getRndValueSqr() * CurPopSize );
    const ptype* const rp4 = AltPop.getParamsOrdered( si4 );
    const ptype* const rp5 = AltPop.getParamsOrdered( CurPopSize1 - si4 );

    int i;

    for( i = 0; i < ParamCount; i++ )
    {
      Params[ i ] = rp1[ i ] - ((( rp3[ i ] - rp2[ i ]) +
        ( rp5[ i ] - rp4[ i ])) >> 1 );
    }
  }

  /**
   * "Centroid mix with DE" solution generator, works well for convex
   * functions. For DE operation, uses a better solution and a random
   * previous solution.
   */

  void generateSol3( CBiteRnd& rnd )
  {
    ptype* const Params = TmpParams;

    const ptype* const MinParams = getParamsOrdered(
      getMinSolIndex( 2, rnd, CurPopSize ));

    if( NeedCentUpdate )
    {
      updateCentroid();
    }

    const ptype* const cp = getCentroid();

    const int si1 = (int) ( rnd.getRndValueSqr() * CurPopSize );
    const ptype* const rp1 = getParamsOrdered( si1 );
    int i;

    for( i = 0; i < ParamCount; i++ )
    {
      Params[ i ] = ( rnd.getBit() ? cp[ i ] :
        MinParams[ i ] + ( MinParams[ i ] - rp1[ i ]));
    }
  }

  /**
   * "Entropy bit mixing"-based solution generator. Performs crossing-over
   * of an odd number (this is important) of random solutions via XOR
   * operation. Slightly less effective than the DE-based mixing, but makes
   * the optimization method more diverse overall.
   */

  void generateSol4( CBiteRnd& rnd )
  {
    ptype* const Params = TmpParams;

    CBiteOptPop& AltPop = selectAltPop( 2, rnd );
    CBiteOptPop& ParPop = selectParPop( 1, rnd );

    int UseSize[ 2 ];
    UseSize[ 0 ] = CurPopSize;
    UseSize[ 1 ] = ParPop.getCurPopSize();

    const ptype* const* UseParams[ 2 ];
    UseParams[ 0 ] = AltPop.getPopParams();
    UseParams[ 1 ] = ParPop.getPopParams();

    const int km = 3 + ( select( Gen4MixFacHist, rnd ) << 1 );

    int p = rnd.getBit();
    int si1 = (int) ( rnd.getRndValueSqr() * UseSize[ p ]);
    const ptype* rp1 = UseParams[ p ][ si1 ];

    memcpy( Params, rp1, ParamCount * sizeof( Params[ 0 ]));

    int k;

    for( k = 1; k < km; k++ )
    {
      p = rnd.getBit();
      si1 = (int) ( rnd.getRndValueSqr() * UseSize[ p ]);
      rp1 = UseParams[ p ][ si1 ];

      int i;

      for( i = 0; i < ParamCount; i++ )
      {
        Params[ i ] ^= rp1[ i ];
      }
    }
  }

  /**
   * Solution generator similar to generateSol4, but uses solutions from the
   * main population only, and includes "crossover" approach first
   * implemented in the generateSol5b() function.
   */

  void generateSol4b( CBiteRnd& rnd )
  {
    ptype* const Params = TmpParams;

    const int km = 3 + ( select( Gen4MixFacHist, rnd ) << 1 );

    int si1 = (int) ( rnd.getRndValueSqr() * CurPopSize );
    const ptype* rp1 = getParamsOrdered( si1 );

    memcpy( Params, rp1, ParamCount * sizeof( Params[ 0 ]));

    int k;

    for( k = 1; k < km; k++ )
    {
      si1 = (int) ( rnd.getRndValueSqr() * CurPopSize );
      int si2 = (int) ( rnd.getRndValueSqr() * CurPopSize );

      const ptype* CrossParams[ 2 ];
      CrossParams[ 0 ] = getParamsOrdered( si1 );
      CrossParams[ 1 ] = getParamsOrdered( si2 );

      int i;

      for( i = 0; i < ParamCount; i++ )
      {
        Params[ i ] ^= CrossParams[ rnd.getBit()][ i ];
      }
    }
  }

  /**
   * A novel "Randomized bit crossing-over" candidate solution generation
   * method. Effective, but on its own cannot stand coordinate system
   * offsets, converges slowly. Completely mixes bits of two
   * randomly-selected solutions, plus changes 1 random bit.
   *
   * This method is fundamentally similar to a biological DNA crossing-over.
   */

  void generateSol5( CBiteRnd& rnd )
  {
    ptype* const Params = TmpParams;

    const CBiteOptPop& ParPop = selectParPop( 2, rnd );

    const ptype* const CrossParams1 = ParPop.getParamsOrdered(
      (int) ( rnd.getRndValueSqr() * ParPop.getCurPopSize() ));

    const CBiteOptPop& AltPop = selectAltPop( 1, rnd );

    const ptype* const CrossParams2 = AltPop.getParamsOrdered(
      (int) ( rnd.getRndValueSqr() * CurPopSize ));

    const bool UseInv = select( Gen5BinvHist, rnd );
    int i;

    for( i = 0; i < ParamCount; i++ )
    {
      // Produce a random bit mixing mask.

      const ptype crpl = (ptype) ( rnd.getUniformRaw2() & IntMantMask );

      ptype v1 = CrossParams1[ i ];
      ptype v2 = ( UseInv && rnd.getBit() ?
        ~CrossParams2[ i ] : CrossParams2[ i ]);

      if( rnd.getBit() )
      {
        const int b = (int) ( rnd.getRndValue() * IntMantBits );

        const ptype m = ~( (ptype) 1 << b );
        const ptype bv = (ptype) rnd.getBit() << b;

        v1 &= m;
        v2 &= m;
        v1 |= bv;
        v2 |= bv;
      }

      Params[ i ] = ( v1 & crpl ) | ( v2 & ~crpl );
    }
  }

  /**
   * "Randomized parameter cross-over" solution generator. Similar to the
   * "randomized bit cross-over", but works with the whole parameter values.
   */

  void generateSol5b( CBiteRnd& rnd )
  {
    ptype* const Params = TmpParams;
    const ptype* CrossParams[ 2 ];

    const CBiteOptPop& ParPop = selectParPop( 2, rnd );

    CrossParams[ 0 ] = ParPop.getParamsOrdered(
      (int) ( rnd.getRndValueSqr() * ParPop.getCurPopSize() ));

    const CBiteOptPop& AltPop = selectAltPop( 1, rnd );

    CrossParams[ 1 ] = AltPop.getParamsOrdered(
      (int) ( rnd.getRndValueSqr() * CurPopSize ));

    int i;

    for( i = 0; i < ParamCount; i++ )
    {
      Params[ i ] = CrossParams[ rnd.getBit()][ i ];
    }
  }

  /**
   * A short-cut solution generator. Parameter value short-cuts: they
   * considerably reduce convergence time for some functions while not
   * severely impacting performance for other functions.
   */

  void generateSol6( CBiteRnd& rnd )
  {
    ptype* const Params = TmpParams;

    const double r = rnd.getRndValueSqr();
    const int si = (int) ( r * r * CurPopSize );
    const double v = getRealValue( getParamsOrdered( si ),
      (int) ( rnd.getRndValue() * ParamCount ));

    int i;

    for( i = 0; i < ParamCount; i++ )
    {
      Params[ i ] = (ptype) (( v - MinValues[ i ]) * DiffValuesI[ i ]);
    }
  }
};

/**
 * Deep optimization class. Based on an array of M CBiteOpt objects. This
 * "deep" method pushes the newly-obtained solution to the next CBiteOpt
 * object which is then optimized.
 *
 * Description is available at https://github.com/avaneev/biteopt
 */

class CBiteOptDeep : public CBiteOptInterface
{
public:
  CBiteOptDeep()
    : ParamCount( 0 )
    , OptCount( 0 )
    , Opts( NULL )
  {
  }

  virtual ~CBiteOptDeep()
  {
    deleteBuffers();
  }

  virtual const double* getBestParams() const
  {
    return( BestOpt -> getBestParams() );
  }

  virtual double getBestCost() const
  {
    return( BestOpt -> getBestCost() );
  }

  /**
   * Function returns a pointer to an array of histograms in use by the
   * current CBiteOpt object.
   */

  CBiteOptHistBase** getHists()
  {
    return( CurOpt -> getHists() );
  }

  /**
   * Function returns a pointer to an array of histogram names.
   */

  const char* const* getHistNames() const
  {
    return( CurOpt -> getHistNames() );
  }

  /**
   * Function returns the number of histograms in use by the current
   * CBiteOpt object.
   */

  int getHistCount() const
  {
    return( CurOpt -> getHistCount() );
  }

  /**
   * Function updates dimensionality of *this object. Function does nothing
   * if dimensionality has not changed since the last call. This function
   * should be called at least once before calling the init() function.
   *
   * @param aParamCount The number of parameters being optimized.
   * @param M The number of CBiteOpt objects. This number depends on the
   * complexity of the problem, if the default value does not produce a good
   * solution, it should be increased together with the iteration count.
   * Minimal value is 1, in this case a plain CBiteOpt optimization will be
   * performed.
   * @param PopSize0 The number of elements in population to use. If set to
   * 0, the default formula will be used.
   */

  void updateDims( const int aParamCount, const int M = 6,
    const int PopSize0 = 0 )
  {
    if( aParamCount == ParamCount && M == OptCount )
    {
      return;
    }

    deleteBuffers();

    ParamCount = aParamCount;
    OptCount = M;
    Opts = new CBiteOptWrap*[ OptCount ];

    int i;

    for( i = 0; i < OptCount; i++ )
    {
      Opts[ i ] = new CBiteOptWrap( this );
      Opts[ i ] -> updateDims( aParamCount, PopSize0 );
    }
  }

  /**
   * Function initializes *this optimizer. Performs N=PopSize objective
   * function evaluations.
   *
   * @param rnd Random number generator.
   * @param InitParams Initial parameter values.
   * @param InitRadius Initial radius, relative to the default value.
   */

  void init( CBiteRnd& rnd, const double* const InitParams = NULL,
    const double InitRadius = 1.0 )
  {
    int i;

    for( i = 0; i < OptCount; i++ )
    {
      Opts[ i ] -> init( rnd, InitParams, InitRadius );
    }

    BestOpt = Opts[ 0 ];
    CurOpt = Opts[ 0 ];
    StallCount = 0;
  }

  /**
   * Function performs the parameter optimization iteration that involves 1
   * objective function evaluation.
   *
   * @param rnd Random number generator.
   * @return The number of non-improving iterations so far. The plateau
   * threshold value is ParamCount * 64.
   */

  int optimize( CBiteRnd& rnd )
  {
    if( OptCount == 1 )
    {
      StallCount = Opts[ 0 ] -> optimize( rnd );

      return( StallCount );
    }

    CBiteOptWrap* PushOpt;

    if( OptCount == 2 )
    {
      PushOpt = Opts[ CurOpt == Opts[ 0 ]];
    }
    else
    {
      while( true )
      {
        PushOpt = Opts[ (int) ( rnd.getRndValue() * OptCount )];

        if( PushOpt != CurOpt )
        {
          break;
        }
      }
    }

    const int sc = CurOpt -> optimize( rnd, PushOpt );

    if( CurOpt -> getBestCost() <= BestOpt -> getBestCost() )
    {
      BestOpt = CurOpt;
    }

    if( sc == 0 )
    {
      StallCount = 0;
    }
    else
    {
      StallCount++;
      CurOpt = PushOpt;
    }

    return( StallCount );
  }

protected:
  /**
   * Wrapper class for CBiteOpt class.
   */

  class CBiteOptWrap : public CBiteOpt
  {
  public:
    CBiteOptDeep* Owner; ///< Owner object.
      ///<

    CBiteOptWrap( CBiteOptDeep* const aOwner )
      : Owner( aOwner )
    {
    }

    virtual void getMinValues( double* const p ) const
    {
      Owner -> getMinValues( p );
    }

    virtual void getMaxValues( double* const p ) const
    {
      Owner -> getMaxValues( p );
    }

    virtual double optcost( const double* const p )
    {
      return( Owner -> optcost( p ));
    }
  };

  int ParamCount; ///< The total number of internal parameter values in use.
    ///<
  int OptCount; ///< The total number of optimization objects in use.
    ///<
  CBiteOptWrap** Opts; ///< Optimization objects.
    ///<
  CBiteOptWrap* BestOpt; ///< Optimizer that contains the best solution.
    ///<
  CBiteOptWrap* CurOpt; ///< Current optimization object index.
    ///<
  int StallCount; ///< The number of iterations without improvement.
    ///<

  /**
   * Function deletes previously allocated buffers.
   */

  void deleteBuffers()
  {
    if( Opts != NULL )
    {
      int i;

      for( i = 0; i < OptCount; i++ )
      {
        delete Opts[ i ];
      }

      delete[] Opts;
      Opts = NULL;
    }
  }
};

/**
 * Objective function.
 */

typedef double (*biteopt_func)( int N, const double* x,
  void* func_data );

/**
 * Wrapper class for the biteopt_minimize() function.
 */

class CBiteOptMinimize : public CBiteOptDeep
{
public:
  int N; ///< The number of dimensions in objective function.
  biteopt_func f; ///< Objective function.
  void* data; ///< Objective function's data.
  const double* lb; ///< Parameter's lower bounds.
  const double* ub; ///< Parameter's lower bounds.

  virtual void getMinValues( double* const p ) const
  {
    int i;

    for( i = 0; i < N; i++ )
    {
      p[ i ] = lb[ i ];
    }
  }

  virtual void getMaxValues( double* const p ) const
  {
    int i;

    for( i = 0; i < N; i++ )
    {
      p[ i ] = ub[ i ];
    }
  }

  virtual double optcost( const double* const p )
  {
    return( (*f)( N, p, data ));
  }
};

/**
 * Function performs minimization using the CBiteOpt or CBiteOptDeep
 * algorithm.
 *
 * @param N The number of parameters in an objective function.
 * @param f Objective function.
 * @param data Objective function's data.
 * @param lb Lower bounds of obj function parameters, should not be infinite.
 * @param ub Upper bounds of obj function parameters, should not be infinite.
 * @param[out] x Minimizer.
 * @param[out] minf Minimizer's value.
 * @param iter The number of iterations to perform in a single attempt.
 * Corresponds to the number of obj function evaluations that are performed.
 * @param M Depth to use, 1 for plain CBiteOpt algorithm, >1 for CBiteOptDeep
 * algorithm. Expected range is [1; 36]. Internally multiplies "iter" by
 * sqrt(M).
 * @param attc The number of optimization attempts to perform.
 * @param stopc Stopping criteria (convergence check). 0: off, 1: 64*N,
 * 2: 128*N.
 * @return The total number of function evaluations performed; useful if the
 * "stopc" was used.
 */

inline int biteopt_minimize(
    const int N, biteopt_func f, void* data,
  const double* lb, const double* ub, double* x, double* minf,
  const int iter, const int M, const int attc,
    const int stopc,
    const int random_seed)
{
  CBiteOptMinimize opt;
  opt.N = N;
  opt.f = f;
  opt.data = data;
  opt.lb = lb;
  opt.ub = ub;
  opt.updateDims( N, M );

  CBiteRnd rnd;
  rnd.init( random_seed );

  const int sct = ( stopc <= 0 ? 0 : 64 * N * stopc );
  const int useiter = (int) ( iter * sqrt( (double) M ));
  int evals = 0;
  int k;

  for( k = 0; k < attc; k++ )
  {
    opt.init( rnd );

    int i;

    for( i = 0; i < useiter; i++ )
    {
      const int sc = opt.optimize( rnd );

      if( sct > 0 && sc >= sct )
      {
        evals++;
        break;
      }
    }

    evals += i;

    if( k == 0 || opt.getBestCost() <= *minf )
    {
      memcpy( x, opt.getBestParams(), N * sizeof( x[ 0 ]));
      *minf = opt.getBestCost();
    }
  }

  return( evals );
}

#endif // BITEOPT_INCLUDED


// cc-lib:

#include "opt/opt.h"

using namespace std;

void Opt::internal_minimize(
    const int N, internal_func f, void* data,
    const double* lb, const double* ub, double* x, double* minf,
    const int iter, const int M, const int attc,
    const int random_seed) {
  (void) biteopt_minimize(N, f, (void *)data, lb, ub, x, minf, iter, M, attc,
                          0, random_seed);
}


std::pair<vector<double>, double>
Opt::Minimize(int n,
              const std::function<double(const std::vector<double> &)> &f,
              const std::vector<double> &lower_bound,
              const std::vector<double> &upper_bound,
              int iters,
              int depth,
              int attempts,
              int random_seed) {
  auto wrap_f = [](int n, const double *args, void* data) -> double {
      auto *f = (std::function<double(const std::vector<double> &)> *)data;
      std::vector<double> in(n);
      for (int i = 0; i < n; i++) in[i] = args[i];
      return (*f)(in);
    };
  std::vector<double> out(n);
  double out_v = 0.0;
  Opt::internal_minimize(n, +wrap_f, (void*)&f,
                         lower_bound.data(), upper_bound.data(),
                         out.data(), &out_v, iters, depth, attempts,
                         random_seed);
  return {out, out_v};
}

