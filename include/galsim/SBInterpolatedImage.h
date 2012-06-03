// -*- c++ -*-
#ifndef SBINTERPOLATED_IMAGE_H
#define SBINTERPOLATED_IMAGE_H
/// @file SBInterpolatedImage.h 
/// @brief Contains the class definition for SBInterpolatedImage objects.

#include "TMV.h"

#include "Std.h"
#include "SBProfile.h"
#include "Interpolant.h"
#include "ProbabilityTree.h"

namespace galsim {

    namespace sbp {

        // Magic numbers:
        
        /// FT must be at least this much larger than input
        const double oversample_x = 4.;

    }

    /** 
     * @brief Surface Brightness Profile represented by interpolation over one or more data 
     * tables/images.
     *
     * It is assumed that input images oversample the profiles they represent.  maxK() is set at 
     * the Nyquist frequency of the input image, although it should be noted that interpolants 
     * other than the ideal sinc function may make the max frequency higher than this.  The output
     * is required to be periodic on a scale > original image extent + kernel footprint, and 
     * stepK() is set accordingly.  Multiple images can be stored as data tables in an 
     * SBInterpolatedImage object. A vector weight can then be used to express Surface 
     * Brightness Profiles as sums of these interpolated images.
     * (TODO: Add more!!!)
     */
    class SBInterpolatedImage : public SBProfile 
    {
    public:
        /**
         * @brief Initialize internal quantities and allocate data tables.
         *
         * @param[in] Npix      extent of square image is `Npix` x `Npix`.
         * @param[in] dx        stepsize between pixels in image data table.
         * @param[in] i         interpolation scheme to adopt between pixels 
         *                      (TODO: Add more, document Interpolant.h, describe the Interpolant2d 
         *                      class).
         * @param[in] Nimages_ number of images.
         */
        SBInterpolatedImage(int Npix, double dx, const Interpolant2d& i, int Nimages=1) :
            SBProfile(new SBInterpolatedImageImpl(Npix,dx,i,Nimages)) {}

        /** 
         * @brief Initialize internal quantities and allocate data tables based on a supplied 2D 
         * image.
         *
         * @param[in] img       square input Image (any of ImageF, ImageD, ImageS, ImageI).
         * @param[in] dx        stepsize between pixels in image data table (default value of 
         *                      `x0_ = 0.` checks the Image header for a suitable stepsize, sets 
         *                      to `1.` if none is found). 
         * @param[in] i         interpolation scheme to adopt between pixels (TODO: Add more, 
         *                      document Interpolant.h, describe the Interpolant2d class).
         * @param[in] padFactor multiple by which to increase the image size when zero-padding for 
         *                      the Fourier transform (default `padFactor = 0.` forces adoption of 
         *                      the currently-hardwired `OVERSAMPLE_X = 4.` parameter value for 
         *                      `padFactor`).
         */
        template <typename T> 
        SBInterpolatedImage(const BaseImage<T>& img, const Interpolant2d& i,
                            double dx=0., double padFactor=0.) :
            SBProfile(new SBInterpolatedImageImpl(img,i,dx,padFactor)) {}

        /// @brief Copy Constructor.
        SBInterpolatedImage(const SBInterpolatedImage& rhs) : SBProfile(rhs) {}

        /// @brief Destructor
        ~SBInterpolatedImage() {}

    protected:

        class SBInterpolatedImageImpl : public SBProfileImpl 
        {
        public:
            SBInterpolatedImageImpl(int Npix, double dx, const Interpolant2d& i, int Nimages);

            template <typename T> 
            SBInterpolatedImageImpl(const BaseImage<T>& img, const Interpolant2d& i,
                                double dx, double padFactor);

            ~SBInterpolatedImageImpl();

            double xValue(const Position<double>& p) const;
            std::complex<double> kValue(const Position<double>& p) const;

            double maxK() const;
            double stepK() const;

            void getXRange(double& xmin, double& xmax, std::vector<double>& ) const 
            { xmin = -_max_size; xmax = _max_size; }

            void getYRange(double& ymin, double& ymax, std::vector<double>& ) const 
            { ymin = -_max_size; ymax = _max_size; }

            bool isAxisymmetric() const { return false; }

            // This class will be set up so that both x and k domain values
            // are found by interpolation of a table:
            bool isAnalyticX() const { return true; }
            bool isAnalyticK() const { return true; }

            Position<double> centroid() const;

            virtual PhotonArray shoot(int N, UniformDeviate& u) const;

            double getFlux() const;

            double getPositiveFlux() const { checkReadyToShoot(); return _positiveFlux; }
            double getNegativeFlux() const { checkReadyToShoot(); return _negativeFlux; }

            void setPixel(double value, int ix, int iy, int iz=0);
            double getPixel(int ix, int iy, int iz=0) const;

            void setWeights(const tmv::Vector<double>& wts); // ??? check dimensions first!
            tmv::Vector<double> getWeights() const { return _wts; }

            void setXInterpolant(const Interpolant2d& interp_) { _xInterp=&interp_; _ready=false; }
            const Interpolant2d& getXInterpolant() const { return *_xInterp; }

            void setKInterpolant(const Interpolant2d& interp_) { _kInterp=&interp_; }
            const Interpolant2d& getKInterpolant() const { return *_kInterp; }

            int getNin() const { return _Ninitial; }

            int getNft() const { return _Nk; }

            template <typename T>
            double fillXImage(ImageView<T>& I, double dx) const;

            // Overrides for better efficiency with separable kernels:
            virtual void fillKGrid(KTable& kt) const;
            virtual void fillXGrid(XTable& xt) const;

            // These are the virtual functions, but we don't want to have to duplicate the
            // code implement these.  So each one just calls the template version.  The
            // C++ overloading rules mean that it will call the local fillXImage template 
            // function defined above, not the one in SBProfile (which would lead to an 
            // infinite loop!). 
            //
            // So here is what happens when someone calls fillXImage(I,dx):
            // 1) If they are calling this from an SBInterpolatedImage object, then
            //    it just directly uses the above template version.
            // 2) If they are calling this from an SBProfile object, the template version
            //    there immediately calls doFillXImage for the appropriate type.
            //    That's a virtual function, so if the SBProfile is really an SBInterpolatedImage,
            //    it will find these virtual functions instead of the ones defined in
            //    SBProfile.  Then these functions immediately call the template version
            //    of fillXImage defined above.
            //
            virtual double doFillXImage(ImageView<float>& I, double dx) const
            { return fillXImage(I,dx); }
            virtual double doFillXImage(ImageView<double>& I, double dx) const
            { return fillXImage(I,dx); }

        private:
            void checkReady() const; ///< Make sure all internal quantities are ok.

            int _Ninitial; ///< Size of input pixel grids.
            double _dx;  ///< Input pixel scales.
            int _Nk;  ///< Size of the padded grids and Discrete Fourier transform table.
            double _dk;  ///< Step size in k for Discrete Fourier transform table.
            int _Nimages; ///< Number of image planes to sum.

            const Interpolant2d* _xInterp; ///< Interpolant used in real space.
            const Interpolant2d* _kInterp; ///< Interpolant used in k space.

            /// @brief Vector of weights to use for sum over images of a multiple image.
            tmv::Vector<double> _wts;

            /// @brief Vector of fluxes for each image plane of a multiple image.
            mutable tmv::Vector<double> _fluxes;

            /// @brief Vector x weighted fluxes for each image plane of a multiple image.
            mutable tmv::Vector<double> _xFluxes;

            /// @brief Vector of y weighted fluxes for each image plane of a multiple image.
            mutable tmv::Vector<double> _yFluxes;

            // Arrays summed with weights:
            mutable XTable* _xsum; ///< Arrays summed with weights in real space.
            mutable KTable* _ksum; ///< Arrays summed with weights in k space.
            mutable bool _xsumValid; ///< Is `xsum` valid?
            mutable bool _ksumValid; ///< Is `ksum` valid?

            /** 
             * @brief Set true if kTables, centroid/flux values,etc., are set for current x pixel 
             * values.
             */
            mutable bool _ready; 

            /// @brief Set true if the data structures for photon-shooting are valid
            mutable bool _readyToShoot;

            /// @brief Set up photon-shooting quantities, if not ready
            void checkReadyToShoot() const;

            // Structures used for photon shooting
            /**
             * @brief Simple structure used to index all pixels for photon shooting
             */
            struct Pixel {
                Pixel(double x_=0., double y_=0., double flux=0.): 
                    x(x_), y(y_), _flux(flux) {isPositive = _flux>=0;}
                double x;
                double y;
                double getFlux() const {return _flux;}
                bool isPositive;
            private:
                double _flux;
            };
            mutable double _positiveFlux;    ///< Sum of all positive pixels' flux
            mutable double _negativeFlux;    ///< Sum of all negative pixels' flux
            mutable ProbabilityTree<Pixel> _pt; ///< Binary tree of pixels, for photon-shooting

            /// @brief Vector of input data arrays.
            std::vector<XTable*> _vx;

            /// @brief Mutable stuff required for kTables and interpolations.
            mutable std::vector<KTable*> _vk;
            void checkXsum() const;  ///< Used to build xsum if it's not current.
            void checkKsum() const;  ///< Used to build ksum if it's not current.

            double _max_size; ///< Calculated value: Ninitial+2*xInterp->xrange())*dx

            // Copy constructor and op= are undefined.
            SBInterpolatedImageImpl(const SBInterpolatedImageImpl& rhs);
            void operator=(const SBInterpolatedImageImpl& rhs);
        };

        /// @brief The default k-space interpolant
        static InterpolantXY defaultKInterpolant2d;

    private:
        // op= is undefined
        void operator=(const SBInterpolatedImage& rhs);
    };
}

#endif // SBINTERPOLATED_IMAGE_H
