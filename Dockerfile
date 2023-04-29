FROM strikef/swpp-ci AS interpreter

RUN git clone https://github.com/snu-sf-class/swpp202301-interpreter interpreter \
 && cd interpreter \
 && ./build.sh

RUN git clone https://github.com/snu-sf-class/swpp202301-benchmarks benchmarks \
 && ./build-lls.py /opt/llvm/bin/clang
