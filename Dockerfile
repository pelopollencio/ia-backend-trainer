# 1. Imagen base con compilador GCC y CMake
FROM ubuntu:22.04 AS builder

# Evitar preguntas interactivas al instalar paquetes
ENV DEBIAN_FRONTEND=noninteractive

# Instalar herramientas de compilación y librerías C++
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-dev \
    libboost-system-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Crear carpeta de trabajo
WORKDIR /app

# Copiar todo el código del repositorio al contenedor
COPY . .

# Compilar la aplicación C++
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# 2. Imagen final ligera de ejecución (Runtime)
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libcurl4 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copiar el binario compilado desde la etapa anterior
COPY --from=builder /app/build/server_app .

# Render asigna dinámicamente un puerto en la variable $PORT (por defecto 18080 si no existe)
ENV PORT=18080
EXPOSE 18080

# Ejecutar el servidor C++
CMD ["./server_app"]