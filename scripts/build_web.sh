#!/usr/bin/env bash

mkdir -p data/assets

npx tailwindcss -i ./web/assets/style.css -o ./web/assets/style.min.css
cat ./web/index.html > data/index.html
cat ./web/settings.html > data/settings.html
cat ./web/ota.html > data/ota.html
cp web/assets/gm.svg data/assets/gm.svg
npx minify ./web/assets/style.min.css > data/assets/style.min.css
sed -i '' 's/%/%%/g' data/assets/style.min.css
