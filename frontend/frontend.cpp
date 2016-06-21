#include "frontend/frontend.h"

namespace frontend {
  FrontendPtr makeDefaultFrontend(NodeManager& manager, const std::string& fragment) {
    return std::make_shared<Converter>(manager, fragment);
  }

  FrontendPtr makeDefaultFrontend(NodeManager& manager, sptr<ast::node> root) {
    return std::make_shared<Converter>(manager, root);
  }
}
