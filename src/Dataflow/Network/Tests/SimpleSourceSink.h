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

#ifndef DATAFLOW_NETWORK_SIMPLESOURCESINK_H
#define DATAFLOW_NETWORK_SIMPLESOURCESINK_H

#include <stdexcept>
#include <Dataflow/Network/DataflowInterfaces.h>

namespace SCIRun
{
  namespace Dataflow
  {
    namespace Networks
    {
      //TODO move to production...
      class SimpleSink : public DatatypeSinkInterface
      {
      public:
        virtual void waitForData()
        {
          //do nothing
        }

        virtual SCIRun::Core::Datatypes::DatatypeHandleOption receive()
        {
          return data_;
        }

        void setData(SCIRun::Core::Datatypes::DatatypeHandle data)
        {
          data_ = data;
        }
      private:
        SCIRun::Core::Datatypes::DatatypeHandle data_;
      };

      class SimpleSource : public DatatypeSourceInterface
      {
      public:
        virtual void send(DatatypeSinkInterfaceHandle receiver, SCIRun::Core::Datatypes::DatatypeHandle data)
        {
          SimpleSink* sink = dynamic_cast<SimpleSink*>(receiver.get());
          if (!sink)
            THROW_INVALID_ARGUMENT("SimpleSource can only send to SimpleSinks");
          sink->setData(data);
        }
      };
    }
  }
}

#endif