# Use an official C++ runtime as a parent image
FROM ubuntu:latest

# Set the working directory in the container to /app
WORKDIR /app

# Copy the current directory contents into the container at /app
ADD . /app

# Install any needed packages specified in requirements.txt
RUN apt-get update && \
    apt-get install -y build-essential && \
    apt-get install -y cmake gcc g++ && \
    apt-get install -y libboost-system-dev libboost-thread-dev && \
    apt-get install -y wget && \
    apt-get install -y git && \
    git clone https://github.com/nlohmann/json.git && \
    cd json && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    make install && \
    cd .. && \
    git clone https://github.com/redis/hiredis.git && \
    cd hiredis && \
    make && \
    make install && \
    cd .. && \
    git clone https://github.com/sewenew/redis-plus-plus.git && \
    cd redis-plus-plus && \
    mkdir compile && \
    cd compile && \
    cmake -DREDIS_PLUS_PLUS_CXX_STANDARD=17 -DREDIS_PLUS_PLUS_BUILD_STATIC=ON -DREDIS_PLUS_PLUS_BUILD_SHARED=OFF .. && \
    make && \
    make install && \
    cd /app && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make

# Add execute permission to the binary
RUN chmod +x /app/build/BoostSync

# Make port 12345 available to the world outside this container
EXPOSE 12345/udp

# Set the LD_LIBRARY_PATH for the runtime linker
ENV LD_LIBRARY_PATH=/usr/local/lib:${LD_LIBRARY_PATH}

# Run BoostSync when the container launches
CMD ["./build/BoostSync"]
