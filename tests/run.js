const fs = require('fs')
const os = require('os')
const path = require('path')
const { execFileSync } = require('child_process')

const dir = process.argv[2] || path.join(__dirname, '..', '..', 'Texel', 'tests', 'textures')
const files = fs.readdirSync(dir).filter(f => /\.(png|tga|exr)$/.test(f)).sort().map(f => path.join(dir, f))
const out = fs.mkdtempSync(path.join(os.tmpdir(), 'libtex-'))
const cc = process.env.CC || 'clang'
const exe = name => path.join(out, name + (process.platform === 'win32' ? '.exe' : ''))
const common = ['-O2', '-std=c11', '-Wall', '-Werror', '-ffp-contract=off']

const wasm = path.join(out, 'hash.wasm')
execFileSync('clang', ['--target=wasm32', '-O3', '-nostdlib', '-ffreestanding', '-mbulk-memory', '-msimd128', '-fvisibility=default', '-Wall', '-Werror',
  '-Wl,--no-entry', '-Wl,--export-dynamic', '-Wl,-z,stack-size=1048576', '-o', wasm, path.join(__dirname, 'wasm.c')], { stdio: 'inherit' })

const builds = { sse: ['-msse4.1'], avx2: ['-mavx2', '-mfma'], scalar: ['-DLIBTEX_SCALAR'] }
for (const [name, flags] of Object.entries(builds))
  execFileSync(cc, [...common, ...flags, '-o', exe(name), path.join(__dirname, 'native.c'), '-lm'], { stdio: 'inherit' })

const instance = new WebAssembly.Instance(new WebAssembly.Module(fs.readFileSync(wasm)), {})
const { memory, init, run, reset, alloc } = instance.exports
init()
const kind = f => f.endsWith('.png') ? 0 : f.endsWith('.tga') ? 1 : 2
const hex = v => (v >>> 0).toString(16).padStart(8, '0')
const lines = ['synthetic ' + hex(run(0, 0, 0))]
for (const f of files) {
  reset()
  const bytes = fs.readFileSync(f)
  const at = alloc(bytes.length)
  new Uint8Array(memory.buffer, at, bytes.length).set(bytes)
  lines.push(path.basename(f) + ' ' + hex(run(at, bytes.length, kind(f))))
}
const want = lines.join('\n')

let bad = 0
for (const name of Object.keys(builds)) {
  const got = execFileSync(exe(name), files, { encoding: 'utf8' }).trim()
  const ok = got === want
  if (!ok) bad++
  console.log(name.padEnd(7), ok ? 'same as wasm' : 'DIFFERENT')
  if (!ok) got.split('\n').forEach((l, i) => l !== lines[i] && console.log('  ', l, 'wasm', lines[i]))
}
console.log(want)
fs.rmSync(out, { recursive: true, force: true })
process.exit(bad ? 1 : 0)
