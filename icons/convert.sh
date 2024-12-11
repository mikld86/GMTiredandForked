#!/usr/bin/env bash


function convert() {
  input="$1"
  base="${input%.*}"
  size="$2"
  output="out/${base}-${size}x${size}.png"
  mkdir -p out

  /Applications/Inkscape.app/Contents/MacOS/inkscape -w "$size" -h "$size" "$input" -o "$output"
}

convert "angle-down.svg" 40
convert "angle-up.svg" 40
convert "bluetooth-alt.svg" 20
convert "coffee-bean.svg" 80
convert "mug-hot-alt.svg" 80
convert "pause.svg" 40
convert "play.svg" 40
convert "power.svg" 40
convert "raindrops.svg" 80
convert "wifi.svg" 20
convert "wind.svg" 80
convert "clock.svg" 40
convert "thermometer-half.svg" 40
