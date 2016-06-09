# Dockerfile for running the muni prediction proxy service.
FROM ubuntu:14.04

RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  ruby2.0 \
  ruby2.0-dev \
  libxml2 \
  zlib1g \
  zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*
RUN gem2.0 install bundler

RUN mkdir -p /app/muniserver/

# Copy the files needed.
COPY simple.rb /app/muniserver/
COPY test.rb /app/muniserver/
COPY config.yaml /app/muniserver/
COPY Gemfile /app/muniserver/

RUN adduser \
  --home=/app/muniserver \
  --no-create-home \
  --disabled-login \
  --gecos "Muni Server user" \
  muniserver
RUN chown -R muniserver /app/muniserver
USER muniserver

WORKDIR /app/muniserver
RUN bundle install --path vendor/bundle

EXPOSE 4567

ENTRYPOINT ["ruby2.0", "/app/muniserver/simple.rb"]
CMD ["--config", "/app/muniserver/config.yaml"]
