/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#ifndef HARD_CODED_MODULE_FACTORY_H
#define HARD_CODED_MODULE_FACTORY_H

#include <Core/Dataflow/Network/ModuleFactory.h>
#include <Modules/Factory/Share.h>

namespace SCIRun {
  namespace Modules {
    namespace Factory {
      
      class SCISHARE HardCodedModuleFactory : public SCIRun::Domain::Networks::ModuleFactory
      {
      public:
        HardCodedModuleFactory();
        virtual SCIRun::Domain::Networks::ModuleDescription lookupDescription(const SCIRun::Domain::Networks::ModuleLookupInfo& info);
        virtual SCIRun::Domain::Networks::ModuleHandle create(const SCIRun::Domain::Networks::ModuleDescription& info);
        virtual void setStateFactory(SCIRun::Domain::Networks::ModuleStateFactoryHandle stateFactory);
        virtual void setRenderer(SCIRun::Domain::Networks::RendererInterface* renderer);
      private:
        SCIRun::Domain::Networks::ModuleStateFactoryHandle stateFactory_;
        SCIRun::Domain::Networks::RendererInterface* renderer_;
      };
    }
  }
}

#endif
