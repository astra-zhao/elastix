#ifndef __itkAdvancedImageToImageMetric_h
#define __itkAdvancedImageToImageMetric_h

#include "itkImageToImageMetric.h"

#include "itkImageSamplerBase.h"
#include "itkForwardGradientImageFilter.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkBSplineDeformableTransform.h"
#include "itkBSplineResampleImageFunction.h"
#include "itkBSplineCombinationTransform.h"
#include "itkLimiterFunctionBase.h"


namespace itk
{
  
/** \class AdvancedImageToImageMetric
 *
 * \brief An extension of the ITK ImageToImageMetric. It is the intended base
 * class for all elastix metrics.
 *
 * This class inherits from the itk::ImageToImageMetric. The additional features of
 * this class that makes it an AdvancedImageToImageMetric are:
 * - the use of an ImageSampler, which selects the fixed image samples over which
 *   the metric is evaluated. In the derived metric you simply need to loop over
 *   the image sample container, instead over the fixed image. This way it is easy
 *   to create different samplers, without the derived metric needing to know.
 * - limiters
 * - differential overlap
 *
 * \ingroup RegistrationMetrics
 *
 */

template <class TFixedImage, class TMovingImage>
class AdvancedImageToImageMetric :
  public ImageToImageMetric< TFixedImage, TMovingImage >
{
public:
  /** Standard class typedefs. */
  typedef AdvancedImageToImageMetric  Self;
  typedef ImageToImageMetric<
    TFixedImage, TMovingImage >           Superclass;
  typedef SmartPointer<Self>              Pointer;
  typedef SmartPointer<const Self>        ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro( AdvancedImageToImageMetric, ImageToImageMetric );

  /** Constants for the image dimensions. */
  itkStaticConstMacro( MovingImageDimension, unsigned int,
    TMovingImage::ImageDimension );
  itkStaticConstMacro( FixedImageDimension, unsigned int,
    TFixedImage::ImageDimension );

  /** Typedefs from the superclass. */
  typedef typename 
    Superclass::CoordinateRepresentationType              CoordinateRepresentationType;
  typedef typename Superclass::MovingImageType            MovingImageType;
  typedef typename Superclass::MovingImagePixelType       MovingImagePixelType;
  typedef typename Superclass::MovingImageConstPointer    MovingImageConstPointer;
  typedef typename Superclass::FixedImageType             FixedImageType;
  typedef typename Superclass::FixedImageConstPointer     FixedImageConstPointer;
  typedef typename Superclass::FixedImageRegionType       FixedImageRegionType;
  typedef typename Superclass::TransformType              TransformType;
  typedef typename Superclass::TransformPointer           TransformPointer;
  typedef typename Superclass::InputPointType             InputPointType;
  typedef typename Superclass::OutputPointType            OutputPointType;
  typedef typename Superclass::TransformParametersType    TransformParametersType;
  typedef typename Superclass::TransformJacobianType      TransformJacobianType;
  typedef typename Superclass::InterpolatorType           InterpolatorType;
  typedef typename Superclass::InterpolatorPointer        InterpolatorPointer;
  typedef typename Superclass::RealType                   RealType;
  typedef typename Superclass::GradientPixelType          GradientPixelType;
  typedef typename Superclass::GradientImageType          GradientImageType;
  typedef typename Superclass::GradientImagePointer       GradientImagePointer;
  typedef typename Superclass::GradientImageFilterType    GradientImageFilterType;
  typedef typename Superclass::GradientImageFilterPointer GradientImageFilterPointer;
  typedef typename Superclass::FixedImageMaskType         FixedImageMaskType;
  typedef typename Superclass::FixedImageMaskPointer      FixedImageMaskPointer;
  typedef typename Superclass::MovingImageMaskType        MovingImageMaskType;
  typedef typename Superclass::MovingImageMaskPointer     MovingImageMaskPointer;
  typedef typename Superclass::MeasureType                MeasureType;
  typedef typename Superclass::DerivativeType             DerivativeType;
  typedef typename Superclass::ParametersType             ParametersType;

  /** Some useful extra typedefs. */
  typedef typename FixedImageType::PixelType              FixedImagePixelType;
  typedef typename MovingImageType::RegionType            MovingImageRegionType;

  /** Typedefs for the ImageSampler. */
  typedef ImageSamplerBase< FixedImageType >              ImageSamplerType;
  typedef typename ImageSamplerType::Pointer              ImageSamplerPointer;
  typedef typename 
    ImageSamplerType::OutputVectorContainerType           ImageSampleContainerType;
  typedef typename 
    ImageSamplerType::OutputVectorContainerPointer        ImageSampleContainerPointer;
  
  /** Typedefs for smooth differentiable mask support. */
  typedef unsigned char                                   InternalMaskPixelType;
  typedef typename itk::Image<
    InternalMaskPixelType, 
    itkGetStaticConstMacro(MovingImageDimension) >        InternalMovingImageMaskType;
  typedef itk::BSplineResampleImageFunction<
    InternalMovingImageMaskType,
    CoordinateRepresentationType >                        MovingImageMaskInterpolatorType;

  /** Typedefs for Limiter support. */
  typedef LimiterFunctionBase<
    RealType, FixedImageDimension>                        FixedImageLimiterType;
  typedef typename FixedImageLimiterType::OutputType      FixedImageLimiterOutputType;
  typedef LimiterFunctionBase<
    RealType, MovingImageDimension>                       MovingImageLimiterType;
  typedef typename MovingImageLimiterType::OutputType     MovingImageLimiterOutputType;

  /** Public methods ********************/

  /** Set/Get the image sampler. */
  itkSetObjectMacro( ImageSampler, ImageSamplerType );
  virtual ImageSamplerType * GetImageSampler( void ) const
  {
    return this->m_ImageSampler.GetPointer();
  };

  /** Inheriting classes can specify whether they use the image sampler functionality;
   * This method allows the user to inspect this setting. */ 
  itkGetConstMacro( UseImageSampler, bool );

  /** Set/Get the required ratio of valid samples; default 0.25.
   * When less than this ratio*numberOfSamplesTried samples map 
   * inside the moving image buffer, an exception will be thrown. */
  itkSetMacro( RequiredRatioOfValidSamples, double );
  itkGetConstMacro( RequiredRatioOfValidSamples, double );

  /** Set/Get whether the overlap should be taken into account while computing the derivative
   * This setting also affects the value of the metric. Default: false; */
  itkSetMacro( UseDifferentiableOverlap, bool );
  itkGetConstMacro( UseDifferentiableOverlap, bool );

  /** Set the interpolation spline order for the moving image mask; default: 2
   * Make sure to call this before calling Initialize(), if you want to change it. */
  virtual void SetMovingImageMaskInterpolationOrder( unsigned int order )
  {
    this->m_MovingImageMaskInterpolator->SetSplineOrder( order );
  };
  /** Get the interpolation spline order for the moving image mask. */
  virtual const unsigned int GetMovingImageMaskInterpolationOrder( void ) const
  {
    return this->m_MovingImageMaskInterpolator->GetSplineOrder();
  };

  /** Get the internal moving image mask. Equals the movingimage mask if set, and 
   * otherwise it's a box with size equal to the moving image's largest possible region. */
  itkGetConstObjectMacro( InternalMovingImageMask, InternalMovingImageMaskType );

  /** Get the interpolator of the internal moving image mask. */
  itkGetConstObjectMacro( MovingImageMaskInterpolator, MovingImageMaskInterpolatorType );

  /** Set/Get the Moving/Fixed limiter. Its thresholds and bounds are set by the metric. 
   * Setting a limiter is only mandatory if GetUse{Fixed,Moving}Limiter() returns true. */ 
  itkSetObjectMacro( MovingImageLimiter, MovingImageLimiterType );
  itkGetConstObjectMacro( MovingImageLimiter, MovingImageLimiterType );
  itkSetObjectMacro( FixedImageLimiter, FixedImageLimiterType );
  itkGetConstObjectMacro( FixedImageLimiter, FixedImageLimiterType );

  /** A percentage that defines how much the gray value range is extended
   * maxlimit = max + LimitRangeRatio * (max - min)
   * minlimit = min - LimitRangeRatio * (max - min)
   * Default: 0.01;
   * If you use a nearest neighbor or linear interpolator,
   * set it to zero and use a hard limiter. */
  itkSetMacro( MovingLimitRangeRatio, double );
  itkGetConstMacro( MovingLimitRangeRatio, double );
  itkSetMacro( FixedLimitRangeRatio, double );
  itkGetConstMacro( FixedLimitRangeRatio, double );
  
  /** Inheriting classes can specify whether they use the image limiter functionality.
   * This method allows the user to inspect this setting. */ 
  itkGetConstMacro( UseFixedImageLimiter, bool );
  itkGetConstMacro( UseMovingImageLimiter, bool );
  
  /** Initialize the Metric by making sure that all the components
   *  are present and plugged together correctly.
   * \li Call the superclass' implementation
   * \li Cache the number of transform parameters
   * \li Initialize the image sampler, if used.
   * \li Check if a bspline interpolator has been set
   * \li Check if a BSplineTransform or a BSplineCombinationTransform has been set
   * \li Initialize the internal (smooth) mask, if used. */
  virtual void Initialize(void) throw ( ExceptionObject );

protected:
  AdvancedImageToImageMetric();
  virtual ~AdvancedImageToImageMetric() {};
  void PrintSelf( std::ostream& os, Indent indent ) const;

  /** Protected Typedefs ******************/
  
  /** Typedefs for indices and points. */
	typedef typename FixedImageType::IndexType                    FixedImageIndexType;
	typedef typename FixedImageIndexType::IndexValueType          FixedImageIndexValueType;
	typedef typename MovingImageType::IndexType                   MovingImageIndexType;
	typedef typename TransformType::InputPointType                FixedImagePointType;
	typedef typename TransformType::OutputPointType               MovingImagePointType;
  typedef typename InterpolatorType::ContinuousIndexType        MovingImageContinuousIndexType;
	
  /** Typedefs used for computing image derivatives. */
	typedef	BSplineInterpolateImageFunction<
    MovingImageType, CoordinateRepresentationType>              BSplineInterpolatorType;
  typedef typename BSplineInterpolatorType::CovariantVectorType MovingImageDerivativeType;
  typedef ForwardGradientImageFilter<
    MovingImageType, RealType, RealType>                        ForwardDifferenceFilterType;
      
  /** Typedefs for support of sparse jacobians and BSplineTransforms. */
  enum { DeformationSplineOrder = 3 };
	typedef BSplineDeformableTransform<
		CoordinateRepresentationType,
		itkGetStaticConstMacro(FixedImageDimension),
		DeformationSplineOrder>													            BSplineTransformType;
  typedef typename 
		BSplineTransformType::WeightsType								            BSplineTransformWeightsType;
	typedef typename 
		BSplineTransformType::ParameterIndexArrayType 	            BSplineTransformIndexArrayType;
	typedef itk::BSplineCombinationTransform<
		CoordinateRepresentationType,
		itkGetStaticConstMacro(FixedImageDimension),
		DeformationSplineOrder>													            BSplineCombinationTransformType;
 	typedef FixedArray< unsigned long, 
		itkGetStaticConstMacro(FixedImageDimension)>                BSplineParametersOffsetType;
  /** Array type for holding parameter indices */
  typedef Array<unsigned int>                                   ParameterIndexArrayType;
  
  /** Typedefs for smooth differentiable mask support. */
  typedef typename 
    MovingImageMaskInterpolatorType::CovariantVectorType        MovingImageMaskDerivativeType;
  
  /** Protected Variables **************/

  /** Variables for ImageSampler support. m_ImageSampler is mutable, because it is
   * changed in the GetValue(), etc, which are const function. */
  mutable ImageSamplerPointer   m_ImageSampler;

  /** Variables for image derivative computation. */
	bool m_InterpolatorIsBSpline;
	typename BSplineInterpolatorType::Pointer             m_BSplineInterpolator;
	typename ForwardDifferenceFilterType::Pointer         m_ForwardDifferenceFilter;

  /** Variables used when the transform is a bspline transform. */
  bool m_TransformIsBSpline;
	bool m_TransformIsBSplineCombination;
  typename BSplineTransformType::Pointer						m_BSplineTransform;
	mutable BSplineTransformWeightsType								m_BSplineTransformWeights;
	mutable BSplineTransformIndexArrayType						m_BSplineTransformIndices;
	typename BSplineCombinationTransformType::Pointer m_BSplineCombinationTransform;
	BSplineParametersOffsetType                       m_BSplineParametersOffset;

	/** The number of BSpline parameters per image dimension. */
	long                                              m_NumBSplineParametersPerDim;

	/** The number of BSpline transform weights is the number of
	* of parameter in the support region (per dimension ). */   
	unsigned long                                     m_NumBSplineWeights;

  /** The number of transform parameters. */
  unsigned int m_NumberOfParameters;
  
  /** the parameter indices that have a nonzero jacobian. */
  mutable ParameterIndexArrayType                    m_NonZeroJacobianIndices;

  /** Variables for the internal mask. */
  typename InternalMovingImageMaskType::Pointer      m_InternalMovingImageMask;
  typename MovingImageMaskInterpolatorType::Pointer  m_MovingImageMaskInterpolator;

  /** Variables for the Limiters. */
  typename FixedImageLimiterType::Pointer            m_FixedImageLimiter;
  typename MovingImageLimiterType::Pointer           m_MovingImageLimiter;
  FixedImagePixelType                                m_FixedImageTrueMin;
	FixedImagePixelType                                m_FixedImageTrueMax;
	MovingImagePixelType                               m_MovingImageTrueMin;
	MovingImagePixelType                               m_MovingImageTrueMax;
  FixedImageLimiterOutputType                        m_FixedImageMinLimit;
  FixedImageLimiterOutputType                        m_FixedImageMaxLimit;
  MovingImageLimiterOutputType                       m_MovingImageMinLimit;
  MovingImageLimiterOutputType                       m_MovingImageMaxLimit;
  
  /** Protected methods ************** */

  /** Methods for image sampler support **********/

  /** Initialize variables related to the image sampler; called by Initialize. */
  virtual void InitializeImageSampler( void ) throw ( ExceptionObject );
  
  /** Inheriting classes can specify whether they use the image sampler functionality
   * Make sure to set it before calling Initialize; default: false. */
  itkSetMacro( UseImageSampler, bool );

  /** Check if enough samples have been found to compute a reliable 
   * estimate of the value/derivative; throws an exception if not. */
  virtual void CheckNumberOfSamples(
    unsigned long wanted, unsigned long found, double sumOfMaskValues ) const;
  
  /** Methods for image derivative evaluation support **********/

  /** Initialize variables for image derivative computation; this 
   * method is called by Initialize. */
  virtual void CheckForBSplineInterpolator( void );

  /** Compute the image value (and possibly derivative) at a transformed point.
   * Checks if the point lies within the moving image buffer (bool return).
   * If no gradient is wanted, set the gradient argument to 0.
   * If a BSplineInterpolationFunction is used, this class obtain
	 * image derivatives from the BSpline interpolator. Otherwise, 
	 * image derivatives are computed using (forward) finite differencing. */
  virtual bool EvaluateMovingImageValueAndDerivative( 
    const MovingImagePointType & mappedPoint,
    RealType & movingImageValue,
    MovingImageDerivativeType * gradient ) const;

  /** Methods to support transforms with sparse jacobians, like the BSplineTransform **********/

  /** Check if the transform is a bspline transform. Called by Initialize. */
  virtual void CheckForBSplineTransform( void );

  /** Transform a point from FixedImage domain to MovingImage domain.
	 * This function also checks if mapped point is within support region of the transform.
   * It returns true if so, and false otherwise
   * If the transform is a bspline transform some data is stored (BSplineWeights
   * and BSplineIndices) that speedup EvaluateTransformJacobian. */
  virtual bool TransformPoint( 
    const FixedImagePointType & fixedImagePoint,
	  MovingImagePointType & mappedPoint ) const;
 
  /** This function returns a reference to the transform jacobian.
   * This is either a reference to the full TransformJacobian or
   * a reference to a sparse jacobian. 
   * The m_NonZeroJacobianIndices contains the indices that are nonzero.
   * The length of NonZeroJacobianIndices is set in the CheckForBSplineTransform
   * function. */ 
  virtual const TransformJacobianType & EvaluateTransformJacobian(
    const FixedImagePointType & fixedImagePoint ) const;
   
  /** Methods for support of smooth differentiable masks **********/

  /** Initialize the internal mask; Called by Initialize */
  virtual void InitializeInternalMasks( void );

  /** Estimate value and possibly spatial derivative of internal moving mask;
   * a zero pointer for the derivative will prevent computation of the derivative. */
  virtual void EvaluateMovingMaskValueAndDerivative(
    const MovingImagePointType & point,
    RealType & value,
    MovingImageMaskDerivativeType * derivative ) const;

  /** Methods for the support of gray value limiters. ***************/

  /** Compute the extrema of fixed image over a region 
   * Initializes the m_Fixed[True]{Max,Min}[Limit]
   * This method is called by InitializeLimiters() and uses the FixedLimitRangeRatio */
  virtual void ComputeFixedImageExtrema(
    const FixedImageType * image,
    const FixedImageRegionType & region );   

  /** Compute the extrema of the moving image over a region
   * Initializes the m_Moving[True]{Max,Min}[Limit]
   * This method is called by InitializeLimiters() and uses the MovingLimitRangeRatio; */
  virtual void ComputeMovingImageExtrema(
    const MovingImageType * image,
    const MovingImageRegionType & region );   

  /** Initialize the {Fixed,Moving}[True]{Max,Min}[Limit] and the {Fixed,Moving}ImageLimiter 
   * Only does something when Use{Fixed,Moving}Limiter is set to true; */
  virtual void InitializeLimiters( void );

  /** Inheriting classes can specify whether they use the image limiter functionality
   * Make sure to set it before calling Initialize; default: false. */
  itkSetMacro( UseFixedImageLimiter, bool );
  itkSetMacro( UseMovingImageLimiter, bool );
    
private:
  AdvancedImageToImageMetric(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  bool    m_UseImageSampler;
  bool    m_UseDifferentiableOverlap;
  double  m_FixedLimitRangeRatio;
  double  m_MovingLimitRangeRatio;
  bool    m_UseFixedImageLimiter;
  bool    m_UseMovingImageLimiter;
  double  m_RequiredRatioOfValidSamples;
    
  /** This member should only be directly accessed by the
   * EvaluateTransformJacobian method. */
  mutable TransformJacobianType m_InternalTransformJacobian;
  
}; // end class AdvancedImageToImageMetric

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkAdvancedImageToImageMetric.hxx"
#endif

#endif // end #ifndef __itkAdvancedImageToImageMetric_h

