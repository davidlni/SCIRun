/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2015 Scientific Computing and Imaging Institute,
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

#include <Modules/Visualization/ShowColorMapModule.h>
#include <Core/Datatypes/ColorMap.h>
#include <Core/Datatypes/Color.h>
#include <Core/GeometryPrimitives/Vector.h>
#include <Graphics/Datatypes/GeometryImpl.h>

using namespace SCIRun;
using namespace Modules::Visualization;
using namespace Core::Datatypes;
using namespace Dataflow::Networks;
using namespace Core::Algorithms;
using namespace Core::Geometry;
using namespace Graphics::Datatypes;

std::string ShowColorMapModule::mFSRoot;
std::string ShowColorMapModule::mFSSeparator;

ShowColorMapModule::ShowColorMapModule() : GeometryGeneratingModule(ModuleLookupInfo("ShowColorMap", "Visualization", "SCIRun"))
{
  INITIALIZE_PORT(ColorMapObject);
  INITIALIZE_PORT(GeometryOutput);
  ftInit_ = false;
  ftValid_ = false;
  ftSize_ = 15;
}

void ShowColorMapModule::setStateDefaults()
{
  auto state = get_state();
  state->setValue(DisplaySide, 0);
  state->setValue(DisplayLength, 0);
  state->setValue(TextSize, 2);
  state->setValue(TextRed, 1.);
  state->setValue(TextGreen, 1.);
  state->setValue(TextBlue, 1.);
  state->setValue(Labels, 10);
  state->setValue(Scale, 1.0);
  state->setValue(Units, std::string(""));
  state->setValue(SignificantDigits, 2);
  state->setValue(AddExtraSpace, false);
  state->setValue(XTranslation, 0);
  state->setValue(YTranslation, 0);
}

void ShowColorMapModule::execute()
{
  auto colorMap = getRequiredInput(ColorMapObject);
  if (needToExecute())
  {
    std::ostringstream ostr;
    ostr << get_id() << "_" <<
      colorMap->getColorMapInvert() << colorMap->getColorMapName() << colorMap->getColorMapRescaleScale() <<
      colorMap->getColorMapRescaleShift() << colorMap->getColorMapResolution() << colorMap.get() <<
      colorMap->getColorMapShift();
    auto geom = buildGeometryObject(colorMap, get_state(), ostr.str());
    sendOutput(GeometryOutput, geom);
  }
}

GeometryBaseHandle
ShowColorMapModule::buildGeometryObject(ColorMapHandle cm, ModuleStateHandle state, const std::string& id)
{
  std::vector<Vector> points;
  std::vector<ColorRGB> colors;
  std::vector<uint32_t> indices;
  int32_t numVBOElements = 0;
  double resolution = 1. / static_cast<double>(cm->getColorMapResolution());
  //show colormap does not rescale colors, so reset them. we want to see the whole colormap on the scale.
  ColorMap new_map(cm->getColorStrategy(), cm->getColorMapName(), cm->getColorMapResolution(),
    cm->getColorMapShift(), cm->getColorMapInvert(), 1., 0.);
  for (double i = 0.; std::abs(i - 1.) > 0.000001; i += resolution) {
    ColorRGB col = new_map.valueToColor(i);
    uint32_t offset = static_cast<uint32_t>(points.size());
    points.push_back(Vector(0., i, +0.001));
    colors.push_back(col);
    points.push_back(Vector(1., i, +0.001));
    colors.push_back(col);
    points.push_back(Vector(0., i + resolution, +0.001));
    colors.push_back(col);
    points.push_back(Vector(1., i + resolution, +0.001));
    colors.push_back(col);
    numVBOElements += 2;
    indices.push_back(offset + 0);
    indices.push_back(offset + 1);
    indices.push_back(offset + 3);
    indices.push_back(offset + 3);
    indices.push_back(offset + 2);
    indices.push_back(offset + 0);
  }

  // IBO/VBOs and sizes
  uint32_t iboSize = sizeof(uint32_t) * static_cast<uint32_t>(indices.size());
  uint32_t vboSize = sizeof(float) * 7 * static_cast<uint32_t>(points.size());

  std::shared_ptr<CPM_VAR_BUFFER_NS::VarBuffer> iboBufferSPtr(
    new CPM_VAR_BUFFER_NS::VarBuffer(vboSize));
  std::shared_ptr<CPM_VAR_BUFFER_NS::VarBuffer> vboBufferSPtr(
    new CPM_VAR_BUFFER_NS::VarBuffer(iboSize));

  CPM_VAR_BUFFER_NS::VarBuffer* iboBuffer = iboBufferSPtr.get();
  CPM_VAR_BUFFER_NS::VarBuffer* vboBuffer = vboBufferSPtr.get();

  for (auto a : indices) iboBuffer->write(a);

  for (size_t i = 0; i < points.size(); i++) {
    vboBuffer->write(static_cast<float>(points[i].x()));
    vboBuffer->write(static_cast<float>(points[i].y()));
    vboBuffer->write(static_cast<float>(points[i].z()));
    vboBuffer->write(static_cast<float>(colors[i].r()));
    vboBuffer->write(static_cast<float>(colors[i].g()));
    vboBuffer->write(static_cast<float>(colors[i].b()));
    vboBuffer->write(static_cast<float>(1.f));
  }

  //add the actual points and colors

  auto st = get_state();
  int sigdig = st->getValue(SignificantDigits).toInt();
  int numlabel = st->getValue(Labels).toInt();
  int txtsize = st->getValue(TextSize).toInt();
  double scale = st->getValue(Scale).toDouble();
  int displaySide = state->getValue(DisplaySide).toInt();
  float red = static_cast<float>(st->getValue(TextRed).toDouble());
  float green = static_cast<float>(st->getValue(TextGreen).toDouble());
  float blue = static_cast<float>(st->getValue(TextBlue).toDouble());
  float xTrans = static_cast<float>(st->getValue(XTranslation).toInt());
  float yTrans = static_cast<float>(st->getValue(YTranslation).toInt());
  std::stringstream ss;
  ss << resolution << sigdig << txtsize << numlabel << st->getValue(Units).toString() <<
    scale << displaySide << red << green << blue << xTrans << yTrans;

  std::string uniqueNodeID = id + "colorMapLegend" + ss.str();
  std::string vboName = uniqueNodeID + "VBO";
  std::string iboName = uniqueNodeID + "IBO";
  std::string passName = uniqueNodeID + "Pass";

  // NOTE: Attributes will depend on the color scheme. We will want to
  // normalize the colors if the color scheme is COLOR_IN_SITU.

  // Construct VBO.
  std::string shader = "Shaders/ColorMapLegend";
  std::vector<SpireVBO::AttributeData> attribs;
  attribs.push_back(SpireVBO::AttributeData("aPos", 3 * sizeof(float)));
  attribs.push_back(SpireVBO::AttributeData("aColor", 4 * sizeof(float)));
  std::vector<SpireSubPass::Uniform> uniforms;
  uniforms.push_back(SpireSubPass::Uniform("uXTranslate",static_cast<float>(xTrans)));
  uniforms.push_back(SpireSubPass::Uniform("uYTranslate",static_cast<float>(yTrans)));
  uniforms.push_back(SpireSubPass::Uniform("uDisplaySide", static_cast<float>(displaySide)));
  int displayLength = state->getValue(DisplayLength).toInt();
  uniforms.push_back(SpireSubPass::Uniform("uDisplayLength", static_cast<float>(displayLength)));
  SpireVBO geomVBO = SpireVBO(vboName, attribs, vboBufferSPtr,
    numVBOElements, BBox(), true);

  // Construct IBO.

  SpireIBO geomIBO(iboName, SpireIBO::TRIANGLES, sizeof(uint32_t), iboBufferSPtr);

  RenderState renState;
  renState.set(RenderState::IS_ON, true);
  renState.set(RenderState::HAS_DATA, true);
  
  SpireText text;

  SpireSubPass pass(passName, vboName, iboName, shader,
    COLOR_MAP, renState, RENDER_VBO_IBO, geomVBO, geomIBO, text);

  // Add all uniforms generated above to the pass.
  for (const auto& uniform : uniforms) { pass.addUniform(uniform); }

  GeometryHandle geom(new GeometryObjectSpire(*this, "ShowColorMap"));

  geom->mColorMap = cm->getColorMapName();
  geom->mIBOs.push_back(geomIBO);
  geom->mVBOs.push_back(geomVBO);
  geom->mPasses.push_back(pass);

  //text
  char str2[128];
  std::stringstream sd;
  sd << "%." << sigdig << "f";
  std::vector<Vector> txt_coords;
  double increment = 1. / static_cast<double>(numlabel - 1);
  double textSize = 5. * static_cast<double>(txtsize + 3);
  double dash_size = 18.;
  double pipe_size = 18.;
  size_t text_size = size_t(textSize);
  if (!ftInit_)
  {
    std::string fontPath = mFSRoot + mFSSeparator + "Fonts" + mFSSeparator + "FreeSans.ttf";
    initFreeType(fontPath, text_size);
  }
  else if (!ftValid_)
  {
    std::string fontPath = mFSRoot + mFSSeparator + "Fonts" + mFSSeparator + "FreeSans.ttf";
    loadNewFace(fontPath, text_size);
  }
  if (!ftInit_ || !ftValid_)
    return geom;
  if (ftSize_ != text_size)
  {
    ftSize_ = text_size;
    setFaceSize(ftSize_);
  }
  for (double i = 0.; i <= 1.000000001; i += increment)
  {
    std::stringstream ss;
    std::string oneline;
    sprintf(str2, sd.str().c_str(), i / cm->getColorMapRescaleScale() - cm->getColorMapRescaleShift());
    ss << str2 << " " << st->getValue(Units).toString();
    Vector shift = Vector((displaySide == 0) ?
      (xTrans > 50 ? -(textSize*strlen(ss.str().c_str())) : dash_size) : 0.,
      (displaySide == 0) ?
      0. : (yTrans > 50 ? (-textSize - pipe_size / 2.) : pipe_size), i);

    oneline = ss.str();
    const char *p;
    for (p = oneline.c_str(); *p; p++)
    {
      points.clear();
      indices.clear();
      txt_coords.clear();

      if (FT_Load_Char(ftFace_, *p, FT_LOAD_RENDER))
        continue;
      FT_GlyphSlot g = ftFace_->glyph;

      double x = shift.x() + g->bitmap_left;
      double y = -shift.y() - g->bitmap_top;
      double z = shift.z();
      double w = g->bitmap.width;
      double h = g->bitmap.rows;

      //triangle 1
      points.push_back(Vector(x, -y, z));
      points.push_back(Vector(x+w, -y, z));
      points.push_back(Vector(x, -y-h, z));
      //triangle 2
      points.push_back(Vector(x, -y-h, z));
      points.push_back(Vector(x+w, -y, z));
      points.push_back(Vector(x+w, -y-h, z));
      //triangle 1
      txt_coords.push_back(Vector(0.0, 0.0, 0.0));
      txt_coords.push_back(Vector(1.0, 0.0, 0.0));
      txt_coords.push_back(Vector(0.0, 1.0, 0.0));
      //triangle 2
      txt_coords.push_back(Vector(0.0, 1.0, 0.0));
      txt_coords.push_back(Vector(1.0, 0.0, 0.0));
      txt_coords.push_back(Vector(1.0, 1.0, 0.0));
      //triangle 1
      indices.push_back(0);
      indices.push_back(1);
      indices.push_back(2);
      //triangle 2
      indices.push_back(3);
      indices.push_back(4);
      indices.push_back(5);

      shift += Vector(g->advance.x >> 6, g->advance.y >> 6, 0.0);

      numVBOElements = (uint32_t)points.size();

      // IBO/VBOs and sizes
      iboSize = sizeof(uint32_t) * (uint32_t)indices.size();
      vboSize = sizeof(float) * 5 * (uint32_t)points.size();

      std::shared_ptr<CPM_VAR_BUFFER_NS::VarBuffer> iboBufferSPtr2(
        new CPM_VAR_BUFFER_NS::VarBuffer(vboSize));
      std::shared_ptr<CPM_VAR_BUFFER_NS::VarBuffer> vboBufferSPtr2(
        new CPM_VAR_BUFFER_NS::VarBuffer(iboSize));

      CPM_VAR_BUFFER_NS::VarBuffer* iboBuffer2 = iboBufferSPtr2.get();
      CPM_VAR_BUFFER_NS::VarBuffer* vboBuffer2 = vboBufferSPtr2.get();

      for (auto a : indices) iboBuffer2->write(a);
      for (size_t i = 0; i < points.size(); i++) {
        vboBuffer2->write(static_cast<float>(points[i].x()));
        vboBuffer2->write(static_cast<float>(points[i].y()));
        vboBuffer2->write(static_cast<float>(points[i].z()));
        vboBuffer2->write(static_cast<float>(txt_coords[i].x()));
        vboBuffer2->write(static_cast<float>(txt_coords[i].y()));
      }

      //add the actual points and colors
      uniqueNodeID = id + "colorMapLegendTextFont" +
        static_cast<std::ostringstream*>(&(std::ostringstream()
        << p[0] << x << y << z << w << h))->str();
      vboName = uniqueNodeID + "VBO" +
        static_cast<std::ostringstream*>(&(std::ostringstream()
        << p[0] << x << y << z << w << h))->str();
      iboName = uniqueNodeID + "IBO" +
        static_cast<std::ostringstream*>(&(std::ostringstream()
        << p[0] << x << y << z << w << h))->str();
      passName = uniqueNodeID + "Pass" +
        static_cast<std::ostringstream*>(&(std::ostringstream()
        << p[0] << x << y << z << w << h))->str();

      // NOTE: Attributes will depend on the color scheme. We will want to
      // normalize the colors if the color scheme is COLOR_IN_SITU.

      // Construct VBO.
      shader = "Shaders/ColorMapLegendText";
      attribs.clear();
      attribs.push_back(SpireVBO::AttributeData("aPos", 3 * sizeof(float)));
      attribs.push_back(SpireVBO::AttributeData("aTexCoord", 2 * sizeof(float)));
      uniforms.clear();
      uniforms.push_back(SpireSubPass::Uniform("uXTranslate", static_cast<float>(xTrans)));
      uniforms.push_back(SpireSubPass::Uniform("uYTranslate", static_cast<float>(yTrans)));
      uniforms.push_back(SpireSubPass::Uniform("uDisplaySide", static_cast<float>(displaySide)));
      uniforms.push_back(SpireSubPass::Uniform("uDisplayLength", static_cast<float>(displayLength)));
      uniforms.push_back(SpireSubPass::Uniform("uColor", glm::vec4(red, green, blue, 1.0f)));
      SpireVBO geomVBO2 = SpireVBO(vboName, attribs, vboBufferSPtr2,
        numVBOElements, BBox(), true);

      geom->mVBOs.push_back(geomVBO2);

      // Construct IBO.

      SpireIBO geomIBO2(iboName, SpireIBO::TRIANGLES, sizeof(uint32_t), iboBufferSPtr2);
      geom->mIBOs.push_back(geomIBO2);
      renState.set(RenderState::USE_COLORMAP, false);
      renState.set(RenderState::USE_TRANSPARENCY, true);
      char c[2] = { p[0], 0 };
      SpireText text(c, ftFace_);

      SpireSubPass pass2(passName, vboName, iboName, shader,
        COLOR_MAP, renState, RENDER_VBO_IBO, geomVBO2, geomIBO2, text);

      // Add all uniforms generated above to the pass.
      for (const auto& uniform : uniforms) { pass2.addUniform(uniform); }
      //******************************************************************************************
      // TODO we're not adding this geometry (font) until we debug for it to work on Windows.
      geom->mPasses.push_back(pass2);
      //******************************************************************************************
    }
  }

  return geom;
}

void ShowColorMapModule::initFreeType(const std::string &libName, size_t size)
{
  FT_Error err;
  if (!ftInit_)
  {
    err = FT_Init_FreeType(&ftLib_);
    if (!err)
      ftInit_ = true;
  }

  if (!ftInit_) return;

  err = FT_New_Face(ftLib_, libName.c_str(), 0, &ftFace_);
  if (!err)
    ftValid_ = true;

  if (ftValid_)
  {
    err = FT_Select_Charmap(ftFace_, FT_ENCODING_UNICODE);
    err = FT_Set_Pixel_Sizes(ftFace_, 0, size);
  }
}

void ShowColorMapModule::loadNewFace(const std::string &libName, size_t size)
{
  FT_Error err;
  if (!ftInit_)
  {
    err = FT_Init_FreeType(&ftLib_);
    if (!err)
      ftInit_ = true;
  }

  if (!ftInit_) return;

  if (ftValid_)
  {
    FT_Done_Face(ftFace_);
    ftValid_ = false;
  }

  err = FT_New_Face(ftLib_, libName.c_str(), 0, &ftFace_);
  if (!err)
    ftValid_ = true;

  if (ftValid_)
  {
    err = FT_Select_Charmap(ftFace_, FT_ENCODING_UNICODE);
    err = FT_Set_Pixel_Sizes(ftFace_, 0, size);
  }
}

void ShowColorMapModule::setFaceSize(size_t size)
{
  if (!ftValid_)
    return;

  FT_Set_Pixel_Sizes(ftFace_, 0, size);
}

void ShowColorMapModule::setFSStrings(std::string &root, std::string &separator)
{
  mFSRoot = root;
  mFSSeparator = separator;
}

const AlgorithmParameterName ShowColorMapModule::DisplaySide("DisplaySide");
const AlgorithmParameterName ShowColorMapModule::DisplayLength("DisplayLength");
const AlgorithmParameterName ShowColorMapModule::TextSize("TextSize");
const AlgorithmParameterName ShowColorMapModule::TextColor("TextColor");
const AlgorithmParameterName ShowColorMapModule::Labels("Labels");
const AlgorithmParameterName ShowColorMapModule::Scale("Scale");
const AlgorithmParameterName ShowColorMapModule::Units("Units");
const AlgorithmParameterName ShowColorMapModule::SignificantDigits("SignificantDigits");
const AlgorithmParameterName ShowColorMapModule::AddExtraSpace("AddExtraSpace");
const AlgorithmParameterName ShowColorMapModule::TextRed("TextRed");
const AlgorithmParameterName ShowColorMapModule::TextGreen("TextGreen");
const AlgorithmParameterName ShowColorMapModule::TextBlue("TextBlue");
const AlgorithmParameterName ShowColorMapModule::XTranslation("XTranslation");
const AlgorithmParameterName ShowColorMapModule::YTranslation("YTranslation");
