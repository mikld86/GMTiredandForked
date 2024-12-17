/** @type {import('tailwindcss').Config} */
export default {
  content: ["./src/**/*.{html,js,jsx,tsx,ts}"],
  theme: {
    extend: {},
    fontFamily: {
      logo: ['Montserrat']
    }
  },
  plugins: [],
  safelist: [
    {
      pattern: /^(w|h)-.+/
    }
  ]
}
