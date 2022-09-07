FROM silverlinings89/cpp_build_environment:latest as build

#RUN apt-get update && apt-get install -y cmake gcc-10 g++-10 build-essential

WORKDIR /app
COPY . .
#RUN ln -s /usr/bin/gcc-10 /usr/bin/gcc && ln -s /usr/bin/g++-10 /usr/bin/g++

WORKDIR /app/build

RUN cmake ..
RUN make

FROM ubuntu:latest

RUN groupadd -r app && useradd -r -g app app
USER app

WORKDIR /app

COPY --from=build /app/build/bin/main .

ENTRYPOINT ["./main"]