#pragma once
#include "frontend/converter.h"

namespace frontend {
  typedef Ptr<Converter> FrontendPtr;

  FrontendPtr makeDefaultFrontend(NodeManager& manager, const std::string& fragment);
  FrontendPtr makeDefaultFrontend(NodeManager& manager, sptr<ast::node> root);
}
