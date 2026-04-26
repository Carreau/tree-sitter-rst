SPHINXBUILD = sphinx-build
SOURCEDIR   = .
BUILDDIR    = _build

.PHONY: help html clean

help:
	@echo "make html   - build HTML output"
	@echo "make clean  - remove build directory"

html:
	$(SPHINXBUILD) -b html $(SOURCEDIR) $(BUILDDIR)/html
	@echo "Build finished. Open $(BUILDDIR)/html/index.html"

clean:
	rm -rf $(BUILDDIR)
