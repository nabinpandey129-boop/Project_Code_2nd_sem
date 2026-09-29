# Regenerates web/Page.h from web/index.html (only needed if you edit index.html)
html = open("web/index.html", encoding="utf-8").read()
open("web/Page.h", "w", encoding="utf-8").write('#pragma once\nstatic const char* kPage = R"HMSPAGE(' + html + ')HMSPAGE";\n')
