/*
Software License :

Copyright (c) 2007, The Foundry Visionmongers Ltd. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name The Foundry Visionmongers Ltd, nor the names of its 
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <assert.h>

// ofx
#include "ofxCore.h"

// ofx host
#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhClip.h"
#include "ofxhImageEffect.h"

namespace OFX {

  namespace Host {

    namespace ImageEffect {

      /// properties common to the desciptor and instance
      /// the desc and set them, the instance cannot
      static Property::PropSpec clipDescriptorStuffs[] = {
        { kOfxPropType, Property::eString, 1, true, kOfxTypeClip },
        { kOfxPropName, Property::eString, 1, true, "SET ME ON CONSTRUCTION" },
        { kOfxPropLabel, Property::eString, 1, false, "" } ,
        { kOfxPropShortLabel, Property::eString, 1, false, "" },
        { kOfxPropLongLabel, Property::eString, 1, false, "" },        
        { kOfxImageEffectPropSupportedComponents, Property::eString, 0, false, "" },
        { kOfxImageEffectPropTemporalClipAccess,   Property::eInt, 1, false, "0" },
        { kOfxImageClipPropOptional, Property::eInt, 1, false, "0" },
        { kOfxImageClipPropIsMask,   Property::eInt, 1, false, "0" },
        { kOfxImageClipPropFieldExtraction, Property::eString, 1, false, kOfxImageFieldDoubled },
        { kOfxImageEffectPropSupportsTiles,   Property::eInt, 1, false, "1" },  
        { 0 },
      };
      
      ////////////////////////////////////////////////////////////////////////////////
      // props to clips descriptors and instances

      // base ctor, for a descriptor
      ClipBase::ClipBase()
        : _properties(clipDescriptorStuffs) 
      {
      }

      /// props to clips and 
      ClipBase::ClipBase(const ClipBase &v)
        : _properties(v._properties) 
      {
        /// we are an instance, we need to reset the props to read only
        const Property::PropertyMap &map = _properties.getProperties();
        Property::PropertyMap::const_iterator i;
        for(i = map.begin(); i != map.end(); ++i) {
          (*i).second->setPluginReadOnly(false);
        } 
      }

      /// name of the clip
      const std::string &ClipBase::getShortLabel() const
      {
        const std::string &s = _properties.getStringProperty(kOfxPropShortLabel);
        if(s == "") {
          return getLabel();
        }
        return s;
      }
      
      /// name of the clip
      const std::string &ClipBase::getLabel() const
      {
        const std::string &s = _properties.getStringProperty(kOfxPropLabel);
        if(s == "") {
          return getName();
        }
        return s;
      }
      
      /// name of the clip
      const std::string &ClipBase::getLongLabel() const
      {
        const std::string &s = _properties.getStringProperty(kOfxPropLongLabel);
        if(s == "") {
          return getLabel();
        }
        return s;
      }
      
      /// return a std::vector of supported comp
      const std::vector<std::string> &ClipBase::getSupportedComponents() const
      {
        Property::String *p =  _properties.fetchStringProperty(kOfxImageEffectPropSupportedComponents);
        assert(p != NULL);
        return p->getValues();
      }
      
      /// is the given component supported
      bool ClipBase::isSupportedComponent(const std::string &comp) const
      {
        return _properties.findStringPropValueIndex(kOfxImageEffectPropSupportedComponents, comp) != -1;
      }
      
      /// does the clip do random temporal access
      bool ClipBase::temporalAccess() const
      {
        return _properties.getIntProperty(kOfxImageEffectPropTemporalClipAccess);
      }
      
      /// is the clip optional
      bool ClipBase::isOptional() const
      {
        return _properties.getIntProperty(kOfxImageClipPropOptional);
      }
      
      /// is the clip a nominal 'mask' clip
      bool ClipBase::isMask() const
      {
        return _properties.getIntProperty(kOfxImageClipPropIsMask);
      }
      
      /// how does this clip like fielded images to be presented to it
      const std::string &ClipBase::getFieldExtraction()
      {
        return _properties.getStringProperty(kOfxImageClipPropFieldExtraction);
      }
      
      /// is the clip a nominal 'mask' clip
      bool ClipBase::supportsTiles() const
      {
        return _properties.getIntProperty(kOfxImageEffectPropSupportsTiles);
      }

      Property::Set& ClipBase::getProps() 
      {
        return _properties;
      }

      /// get a handle on the properties of the clip descriptor for the C api
      OfxPropertySetHandle ClipBase::getPropHandle() 
      {
        return _properties.getHandle();
      }

      /// get a handle on the clip descriptor for the C api
      OfxImageClipHandle ClipBase::getHandle() 
      {
        return (OfxImageClipHandle)this;
      }

      ////////////////////////////////////////////////////////////////////////////////
      /// descriptor
      ClipDescriptor::ClipDescriptor(std::string name) 
        : ClipBase()
      {
        _properties.setStringProperty(kOfxPropName,name);
      }
      
      /// extra properties for the instance, these are fetched from the host
      /// via a get hook and some virtuals
      static Property::PropSpec clipInstanceStuffs[] = { 
        { kOfxImageEffectPropPixelDepth, Property::eString, 1, true, kOfxBitDepthNone },
        { kOfxImageEffectPropComponents, Property::eString, 1, true, kOfxImageComponentNone },
        { kOfxImageClipPropUnmappedPixelDepth, Property::eString, 1, true, kOfxBitDepthNone },
        { kOfxImageClipPropUnmappedComponents, Property::eString, 1, true, kOfxImageComponentNone },
        { kOfxImageEffectPropPreMultiplication, Property::eString, 1, true, kOfxImageOpaque },
        { kOfxImagePropPixelAspectRatio, Property::eDouble, 1, true, "1.0" },
        { kOfxImageEffectPropFrameRate, Property::eDouble, 1, true, "25.0" },
        { kOfxImageEffectPropFrameRange, Property::eDouble, 2, true, "0" },
        { kOfxImageClipPropFieldOrder, Property::eString, 1, true, kOfxImageFieldNone },
        { kOfxImageClipPropConnected, Property::eInt, 1, true, "0" },
        { kOfxImageEffectPropUnmappedFrameRange, Property::eDouble, 2, true, "0" },
        { kOfxImageEffectPropUnmappedFrameRate, Property::eDouble, 1, true, "25.0" },
        { kOfxImageClipPropContinuousSamples, Property::eInt, 1, true, "0" },
        { 0 },
      };


      ////////////////////////////////////////////////////////////////////////////////
      // instance
      ClipInstance::ClipInstance(ImageEffect::Instance* effectInstance, ClipDescriptor& desc) 
        : ClipBase(desc)
        , _effectInstance(effectInstance)
        , _isOutput(desc.isOutput())
        , _pixelDepth(kOfxBitDepthNone)
        , _components(kOfxImageComponentNone)
      {
        // this will a parameters that are needed in an instance but not a 
        // Descriptor
        _properties.addProperties(clipInstanceStuffs);
        int i = 0;
        while(clipInstanceStuffs[i].name) {
          Property::PropSpec& spec = clipInstanceStuffs[i];

          switch (spec.type) {
          case Property::eDouble:
          case Property::eString:
          case Property::eInt:
            _properties.setGetHook(spec.name, this);
            break;
          default:
            break;
          }

          i++;
        }
      }

      // do nothing
      int ClipInstance::getDimension(const std::string &name) OFX_EXCEPTION_SPEC 
      {
        if(name == kOfxImageEffectPropUnmappedFrameRange || name == kOfxImageEffectPropFrameRange)
          return 2;
        return 1;
      }

      // don't know what to do
      void ClipInstance::reset(const std::string &name) OFX_EXCEPTION_SPEC {
        printf("failing in %s\n", __PRETTY_FUNCTION__);
        throw Property::Exception(kOfxStatErrMissingHostFeature);
      }

      // get the virutals for viewport size, pixel scale, background colour
      void ClipInstance::getDoublePropertyN(const std::string &name, double *values, int n) OFX_EXCEPTION_SPEC
      {
        if(name==kOfxImagePropPixelAspectRatio){
          if(n>1) throw Property::Exception(kOfxStatErrValue);
          *values = getAspectRatio();
        }
        else if(name==kOfxImageEffectPropFrameRate){
          if(n>1) throw Property::Exception(kOfxStatErrValue);
          *values = getFrameRate();
        }
        else if(name==kOfxImageEffectPropFrameRange){
          if(n>2) throw Property::Exception(kOfxStatErrValue);
          getFrameRange(values[0], values[1]);
        }
        else if(name==kOfxImageEffectPropUnmappedFrameRate){
          if(n>1) throw Property::Exception(kOfxStatErrValue);
          *values =  getUnmappedFrameRate();
        }
        else if(name==kOfxImageEffectPropUnmappedFrameRange){
          if(n>2) throw Property::Exception(kOfxStatErrValue);
          getUnmappedFrameRange(values[0], values[1]);
        }
        else
          throw Property::Exception(kOfxStatErrValue);
      }

      // get the virutals for viewport size, pixel scale, background colour
      double ClipInstance::getDoubleProperty(const std::string &name, int n) OFX_EXCEPTION_SPEC
      {
        if(name==kOfxImagePropPixelAspectRatio){
          if(n!=0) throw Property::Exception(kOfxStatErrValue);
          return getAspectRatio();
        }
        else if(name==kOfxImageEffectPropFrameRate){
          if(n!=0) throw Property::Exception(kOfxStatErrValue);
          return getFrameRate();
        }
        else if(name==kOfxImageEffectPropFrameRange){
          if(n>1) throw Property::Exception(kOfxStatErrValue);
          double range[2];
          getFrameRange(range[0], range[1]);
          return range[n];
        }
        else if(name==kOfxImageEffectPropUnmappedFrameRate){
          if(n>0) throw Property::Exception(kOfxStatErrValue);
          return getUnmappedFrameRate();
        }
        else if(name==kOfxImageEffectPropUnmappedFrameRange){
          if(n>1) throw Property::Exception(kOfxStatErrValue);
          double range[2];
          getUnmappedFrameRange(range[0], range[1]);
          return range[n];
        }
        else
          throw Property::Exception(kOfxStatErrValue);
      }

      // get the virutals for viewport size, pixel scale, background colour
      int ClipInstance::getIntProperty(const std::string &name, int n) OFX_EXCEPTION_SPEC
      {
        if(n!=0) throw Property::Exception(kOfxStatErrValue);
        if(name==kOfxImageClipPropConnected){
          return getConnected();
        }
        else if(name==kOfxImageClipPropContinuousSamples){
          return getContinuousSamples();
        }
        else
          throw Property::Exception(kOfxStatErrValue);
      }

      // get the virutals for viewport size, pixel scale, background colour
      void ClipInstance::getIntPropertyN(const std::string &name, int *values, int n) OFX_EXCEPTION_SPEC
      {
        if(n!=0) throw Property::Exception(kOfxStatErrValue);
        *values = getIntProperty(name, 0);
      }

      // get the virutals for viewport size, pixel scale, background colour
      const std::string &ClipInstance::getStringProperty(const std::string &name, int n) OFX_EXCEPTION_SPEC
      {
        if(n!=0) throw Property::Exception(kOfxStatErrValue);
        if(name==kOfxImageEffectPropPixelDepth){
          return getPixelDepth();
        }
        else if(name==kOfxImageEffectPropComponents){
          return getComponents();
        }
        else if(name==kOfxImageClipPropUnmappedPixelDepth){
          return getUnmappedBitDepth();
        }
        else if(name==kOfxImageClipPropUnmappedComponents){
          return getUnmappedComponents();
        }
        else if(name==kOfxImageEffectPropPreMultiplication){
          return getPremult();
        }
        else if(name==kOfxImageClipPropFieldOrder){
          return getFieldOrder();
        }
        else
          throw Property::Exception(kOfxStatErrValue);
      }

      // notify override properties
      void ClipInstance::notify(const std::string &name, bool isSingle, int indexOrN)  OFX_EXCEPTION_SPEC
      {
      }

      OfxStatus ClipInstance::instanceChangedAction(std::string why,
                                                OfxTime     time,
                                                OfxPointD   renderScale)
      {
        Property::PropSpec stuff[] = {
          { kOfxPropType, Property::eString, 1, true, kOfxTypeClip },
          { kOfxPropName, Property::eString, 1, true, getName().c_str() },
          { kOfxPropChangeReason, Property::eString, 1, true, why.c_str() },
          { kOfxPropTime, Property::eDouble, 1, true, "0" },
          { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "0" },
          { 0 }
        };

        Property::Set inArgs(stuff);

        // add the second dimension of the render scale
        inArgs.setDoubleProperty(kOfxPropTime,time);
        inArgs.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);

        if(_effectInstance){
          return _effectInstance->mainEntry(kOfxActionBeginInstanceChanged, _effectInstance->getHandle(),inArgs.getHandle(),0);
        }

        return kOfxStatFailed;
      }

      /// given the colour component, find the nearest set of supported colour components
      const std::string &ClipInstance::findSupportedComp(const std::string &s)
      { 
        static const std::string none(kOfxImageComponentNone);
        static const std::string rgba(kOfxImageComponentRGBA);
        static const std::string alpha(kOfxImageComponentAlpha);

        /// is it there
        if(isSupportedComponent(s))
          return s;

        /// were we fed some custom non chromatic component by getUnmappedComponents? Return it. 
        /// we should never be here mind, so a bit weird
        if(!_effectInstance->isChromaticComponent(s))
          return s;

        /// Means we have RGBA or Alpha being passed in and the clip
        /// only supports the other one, so return that
        if(s == kOfxImageComponentRGBA && isSupportedComponent(kOfxImageComponentAlpha))
          return alpha;

        if(s == kOfxImageComponentAlpha && isSupportedComponent(kOfxImageComponentRGBA))
          return rgba;
        
        /// wierd, must be some custom bit , if only one, choose that, otherwise no idea
        /// how to map, you need to derive to do so.
        const std::vector<std::string> &supportedComps = getSupportedComponents();
        if(supportedComps.size() == 1)
          return supportedComps[0];

        return none;
      }
      
      
      ////////////////////////////////////////////////////////////////////////////////
      // Image
      //

      static Property::PropSpec imageStuffs[] = {
        { kOfxPropType, Property::eString, 1, false, kOfxTypeImage },
        { kOfxImageEffectPropPixelDepth, Property::eString, 1, true, kOfxBitDepthNone  },
        { kOfxImageEffectPropComponents, Property::eString, 1, true, kOfxImageComponentNone },
        { kOfxImageEffectPropPreMultiplication, Property::eString, 1, true, kOfxImageOpaque  },
        { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "1.0" },
        { kOfxImagePropPixelAspectRatio, Property::eDouble, 1, true, "1.0"  },
        { kOfxImagePropData, Property::ePointer, 1, true, NULL },
        { kOfxImagePropBounds, Property::eInt, 4, true, "0" },
        { kOfxImagePropRegionOfDefinition, Property::eInt, 4, true, "0", },
        { kOfxImagePropRowBytes, Property::eInt, 1, true, "0", },
        { kOfxImagePropField, Property::eString, 1, true, "", },
        { kOfxImagePropUniqueIdentifier, Property::eString, 1, true, "" },
        { 0 }
      };

      Image::Image()
        : Property::Set(imageStuffs) 
        , _referenceCount(1)
      {
      }

      /// called during ctor to get bits from the clip props into ours
      void Image::getClipBits(ClipInstance& instance)
      {
        Property::Set& clipProperties = instance.getProps();
        
        // get and set the clip instance pixel depth
        const std::string &depth = clipProperties.getStringProperty(kOfxImageEffectPropPixelDepth);
        setStringProperty(kOfxImageEffectPropPixelDepth, depth); 
        
        // get and set the clip instance components
        const std::string &comps = clipProperties.getStringProperty(kOfxImageEffectPropComponents);
        setStringProperty(kOfxImageEffectPropComponents, comps);
        
        // get and set the clip instance premultiplication
        setStringProperty(kOfxImageEffectPropPreMultiplication, clipProperties.getStringProperty(kOfxImageEffectPropPreMultiplication));

        // get and set the clip instance pixel aspect ratio
        setDoubleProperty(kOfxImagePropPixelAspectRatio, clipProperties.getDoubleProperty(kOfxImagePropPixelAspectRatio));
        
        // get and set the clip instance pixel aspect ratio
        setDoubleProperty(kOfxImagePropPixelAspectRatio, clipProperties.getDoubleProperty(kOfxImagePropPixelAspectRatio));
      }

      /// make an image from a clip instance
      Image::Image(ClipInstance& instance)
        : Property::Set(imageStuffs) 
        , _referenceCount(1)
      {
        getClipBits(instance);
      }      

      // construction based on clip instance
      Image::Image(ClipInstance& instance,
                   double renderScaleX, 
                   double renderScaleY,
                   void* data,
                   const OfxRectI &bounds,
                   const OfxRectI &rod,
                   int rowBytes,
                   std::string field,
                   std::string uniqueIdentifier) 
        : Property::Set(imageStuffs)
        , _referenceCount(1)
      {
        getClipBits(instance);

        // set other data
        setDoubleProperty(kOfxImageEffectPropRenderScale,renderScaleX, 0);    
        setDoubleProperty(kOfxImageEffectPropRenderScale,renderScaleY, 1);        
        setPointerProperty(kOfxImagePropData,data);
        setIntProperty(kOfxImagePropBounds,bounds.x1, 0);
        setIntProperty(kOfxImagePropBounds,bounds.y1, 1);
        setIntProperty(kOfxImagePropBounds,bounds.x2, 2);
        setIntProperty(kOfxImagePropBounds,bounds.y2, 3);
        setIntProperty(kOfxImagePropRegionOfDefinition,rod.x1, 0);
        setIntProperty(kOfxImagePropRegionOfDefinition,rod.y1, 1);
        setIntProperty(kOfxImagePropRegionOfDefinition,rod.x2, 2);
        setIntProperty(kOfxImagePropRegionOfDefinition,rod.y2, 3);        
        setIntProperty(kOfxImagePropRowBytes,rowBytes);
        
        setStringProperty(kOfxImagePropField,field);
        setStringProperty(kOfxImageClipPropFieldOrder,field);
        setStringProperty(kOfxImagePropUniqueIdentifier,uniqueIdentifier);
      }

      Image::~Image() {
      }

      // release the reference 
      void Image::releaseReference()
      {
        _referenceCount -= 1;
        if(_referenceCount <= 0)
          delete this;
      }

    } // Clip

  } // Host

} // OFX
