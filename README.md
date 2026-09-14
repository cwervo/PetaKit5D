# PetaKit5D static timeline viewer

This fork turns the repository into a small static C++ + WASM viewer for PetaKit5D timeline metadata.
It parses two lightweight syntaxes:

- Tcl-style tags: `[petakit5d label {Embryo} src {https://example.org/run.md} x 1024 y 512 z 80 c 2 t 180 fps 24]`
- Byte-language lines: `pk5d:label=Embryo;source=https://example.org/run.md;x=1024;y=512;z=80;c=2;t=180;fps=24`

The viewer accepts metadata from three places:

- URL query parameters in `?q=` and `&s=` syntax
- manual input pasted into the page
- uploaded `.txt` or `.md` files containing either syntax

## Project layout

- `/src`: shared C++ parser, CLI entry point, and WASM export
- `/web`: static HTML, CSS, and JS viewer shell
- `/tests`: focused parser coverage for Tcl tags, byte-language lines, and query decoding
- `/samples`: example `.txt` and `.md` inputs for local testing

## Query-string syntax

`q` and `s` are merged on the client before the C++ parser runs.

- `q=https://.../timeline.md` fetches a remote `.txt` or `.md` document and parses it in the browser
- `q=pk5d:...` treats `q` as inline byte-language or Tcl-style metadata when it is not an HTTP(S) URL
- `s=` always treats the value as inline metadata

Example URLs:

```text
/index.html?q=https://example.org/petakit5d/timeline.md
/index.html?s=%5Bpetakit5d%20label%20%7BEmbryo%7D%20src%20%7Bhttps://example.org/run.md%7D%20x%201024%20y%20512%20z%2080%20c%202%20t%20180%20fps%2024%5D
/index.html?q=pk5d%3Alabel%3DInline%3Bsource%3Dhttps%3A%2F%2Fexample.org%2Frun.txt%3Bx%3D512%3By%3D512%3Bz%3D64%3Bc%3D2%3Bt%3D120%3Bfps%3D30&s=%5Bpetakit5d%20label%20%7BOverlay%7D%20t%2060%20c%201%5D
```

## Supported fields

Both syntaxes support the same keys:

- `label` / `name`
- `src` / `source` / `url`
- `note`
- `x`, `y`, `z`, `c`, `t`
- `fps`
- `ms` or `ms_per_frame`

The browser renders a compact timeline card for each parsed clip and scales the bar width relative to its `t` value. This keeps the viewer static, client-side, and fast enough for small metadata files without any server component.

## Building natively

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/pk5d_cli /home/runner/work/PetaKit5D/PetaKit5D/samples/sample.md
```

## Building the static WASM bundle

Use Emscripten so the same parser can run in the browser:

```bash
emcmake cmake -S . -B build-wasm
cmake --build build-wasm
```

This produces a static site bundle in `build-wasm/dist/` containing:

- `index.html`
- `app.js`
- `styles.css`
- `petakit5d_viewer.js`
- `petakit5d_viewer.wasm`

Open `build-wasm/dist/index.html` with a static file server and pass `?q=` / `&s=` inputs as needed.

## Text and Markdown examples

See the sample inputs in:

- `/samples/sample.txt`
- `/samples/sample.md`
