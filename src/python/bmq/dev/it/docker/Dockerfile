FROM artprod.dev.bloomberg.com/rhel7-dpkg:latest

ENV BMQDOMAINS_REPO     /home/dev/bmqdomains
ENV PYTHONPATH          /home/dev/bmq:/home/dev/bmq/src/applications/bmqbrkr

ADD bbcpu.lst           /bb/bin/bbcpu.lst.bmq
ENV BMQ_HOSTTAGS_FILE   /bb/bin/bbcpu.lst.bmq

ENV PYTHONDONTWRITEBYTECODE 1

# LeakSanitizer does not work under ptrace, so we disable it
ENV ASAN_OPTIONS detect_leaks=0

# Install sanitizers
RUN yum install -y libasan libtsan libubsan

# Install some libs to fix gdb's 'Missing separate debuginfo' warning, and lcov
RUN yum install -y                                                          \
    git devtoolset-7-libstdc++-devel.x86_64 glibc-devel.x86_64 gcov lcov && \
    yum clean all

RUN apt-get update && apt-get install -y --force-yes build-base-cc gdb less \
    python3.8 python3.8-pip
RUN pip3.8 install dumb-init pytest pytest-repeat pytest-xdist==1.34.0 \
    pytest-parallel pytest-rerunfailures filelock

ADD entrypoint.sh       /bb/bin/entrypoint.sh
ENTRYPOINT              ["/bb/bin/entrypoint.sh"]

ENV HOME /home/dev
RUN mkdir -p /home/dev  && \
    chmod a+w /home/dev && \
    echo > $HOME/.gdbinit  \
        "add-auto-load-safe-path /home/dev/bmq/failure-logs/cores/.gdbinit"

# Add an alias for bash
RUN echo "alias gdb='gdb -q'" >> $HOME/.bashrc
