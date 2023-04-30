FROM strikef/swpp-ci AS interpreter

RUN git clone https://github.com/snu-sf-class/swpp202301-interpreter.git interpreter \
 && cd interpreter \
 && ./build.sh \
 && cd ..

RUN git clone https://github.com/snu-sf-class/swpp202301-benchmarks.git benchmarks \
 && cd benchmarks \
 && ./build-lls.py /opt/llvm/bin \
 && cd ..

RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip

RUN ./install-alive2.sh
    
