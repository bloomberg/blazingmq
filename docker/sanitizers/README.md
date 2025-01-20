Ideas:
 - Create Dockerfile that accepts sanitizer name, so can run separately or in parallel
 - Dockerfile
   - ??? should have possibility to reuse cached deps src?
   - two layers: 
      - build instrumented bmq and deps with clang;
      - run instrumented unit tests

 - Use paths `deps` or `thirdparty` for deps
   - check if they present