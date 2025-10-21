class AudioAnalyzer {
  constructor(context) {
    this.context = context;
    this.input = this.context.createGain();
    this.analyser = this.context.createAnalyser();
    this.output = this.context.createGain();

    this.analyser.fftSize = 2048;
    this.analyser.smoothingTimeConstant = 0.8;

    this.input.connect(this.analyser);
    this.analyser.connect(this.output);
    this.output.connect(this.context.destination);

    this.frequencyData = new Uint8Array(this.analyser.frequencyBinCount);
    this.waveformData = new Float32Array(this.analyser.fftSize);

    this.smoothedEnergy = 0;
    this.peakEnergy = 0;
  }

  connectSource(node) {
    node.connect(this.input);
  }

  disconnectSource(node) {
    try {
      node.disconnect(this.input);
    } catch (err) {
      // Ignore disconnect errors when the node is already detached.
    }
  }

  setOutputEnabled(enabled) {
    this.output.gain.value = enabled ? 1 : 0;
  }

  update() {
    this.analyser.getByteFrequencyData(this.frequencyData);
    this.analyser.getFloatTimeDomainData(this.waveformData);

    const len = this.frequencyData.length;
    if (!len) {
      return this.emptyFrame();
    }

    let total = 0;
    let low = 0;
    let mid = 0;
    let high = 0;

    const third = Math.floor(len / 3);
    const twoThirds = third * 2;

    for (let i = 0; i < len; i++) {
      const magnitude = this.frequencyData[i] / 255;
      total += magnitude;
      if (i < third) {
        low += magnitude;
      } else if (i < twoThirds) {
        mid += magnitude;
      } else {
        high += magnitude;
      }
    }

    const average = total / len;
    const lowAvg = third ? low / third : 0;
    const midAvg = third ? mid / third : 0;
    const highAvg = len - twoThirds ? high / (len - twoThirds) : 0;

    // Ease the instantaneous energy to avoid jitter.
    const smoothing = 0.12;
    this.smoothedEnergy += (average - this.smoothedEnergy) * smoothing;
    this.peakEnergy = Math.max(this.peakEnergy * 0.985, this.smoothedEnergy);

    return {
      energy: this.smoothedEnergy,
      peak: this.peakEnergy,
      bands: {
        low: Math.min(lowAvg, 1),
        mid: Math.min(midAvg, 1),
        high: Math.min(highAvg, 1)
      }
    };
  }

  emptyFrame() {
    return {
      energy: 0,
      peak: 0,
      bands: { low: 0, mid: 0, high: 0 }
    };
  }
}

class Particle {
  constructor() {
    this.alive = false;
    this.x = 0;
    this.y = 0;
    this.vx = 0;
    this.vy = 0;
    this.life = 0;
    this.age = 0;
    this.baseSize = 0;
    this.opacity = 0;
    this.r = 1;
    this.g = 1;
    this.b = 1;
    this.twinkle = 0;
  }
}

class ParticleSystem {
  constructor(maxParticles = 2048) {
    this.maxParticles = maxParticles;
    this.particles = Array.from({ length: maxParticles }, () => new Particle());
    this.freeList = [];
    for (let i = maxParticles - 1; i >= 0; i--) {
      this.freeList.push(i);
    }

    this.spawnAccumulator = 0;
    this.baseSpawnRate = 140;
    this.positions = new Float32Array(maxParticles * 2);
    this.colors = new Float32Array(maxParticles * 3);
    this.sizes = new Float32Array(maxParticles);
    this.opacities = new Float32Array(maxParticles);
  }

  update(dt, audioFrame, pixelRatio) {
    const energy = audioFrame?.energy ?? 0;
    const peak = audioFrame?.peak ?? 0;
    const bands = audioFrame?.bands ?? { low: 0, mid: 0, high: 0 };

    const spawnIntensity = this.baseSpawnRate * (0.35 + energy * 1.6 + bands.high * 0.6);
    this.spawnAccumulator += spawnIntensity * dt;

    while (this.spawnAccumulator >= 1) {
      this.spawnAccumulator -= 1;
      this.emitParticle(energy, peak, bands);
    }

    const gravity = 1.35 + bands.low * 1.1;
    const swirl = 0.9 + bands.mid * 1.6;

    for (let i = 0; i < this.maxParticles; i++) {
      const particle = this.particles[i];
      if (!particle.alive) {
        continue;
      }

      particle.age += dt;
      if (particle.age >= particle.life) {
        particle.alive = false;
        this.freeList.push(i);
        continue;
      }

      particle.vy -= gravity * dt;
      particle.vx += Math.sin(particle.age * swirl + particle.twinkle) * 0.05 * dt;

      particle.x += particle.vx * dt;
      particle.y += particle.vy * dt;

      if (particle.y < -1.1 || particle.y > 1.2 || Math.abs(particle.x) > 1.2) {
        particle.alive = false;
        this.freeList.push(i);
        continue;
      }

      const lifeProgress = particle.age / particle.life;
      const fade = Math.max(0, 1 - lifeProgress);
      particle.opacity = fade * (0.65 + energy * 0.5);
    }

    return this.buildDrawData(pixelRatio);
  }

  emitParticle(energy, peak, bands) {
    if (!this.freeList.length) {
      return;
    }

    const index = this.freeList.pop();
    const particle = this.particles[index];

    particle.alive = true;
    particle.age = 0;
    particle.life = 1.4 + Math.random() * 0.9 + energy * 0.6;

    particle.x = (Math.random() * 2 - 1) * (0.18 + bands.mid * 0.08);
    particle.y = -1.05;

    const speed = 1.4 + energy * 2.2 + bands.mid * 0.9;
    const spread = 0.32 + energy * 0.45;
    particle.vx = (Math.random() - 0.5) * spread;
    particle.vy = speed + Math.random() * 0.5;

    const hueShift = bands.high * 0.35 + Math.random() * 0.05;
    const baseColor = this.hslToRgb(0.58 - hueShift, 0.65 + bands.mid * 0.2, 0.55 + energy * 0.25);
    particle.r = baseColor[0];
    particle.g = baseColor[1];
    particle.b = baseColor[2];

    particle.baseSize = 10 + energy * 28 + peak * 18;
    particle.opacity = 1;
    particle.twinkle = Math.random() * Math.PI * 2;
  }

  buildDrawData(pixelRatio = 1) {
    let count = 0;
    const brightener = 0.6;

    for (let i = 0; i < this.maxParticles; i++) {
      const particle = this.particles[i];
      if (!particle.alive) {
        continue;
      }

      const baseIndex = count * 2;
      this.positions[baseIndex] = particle.x;
      this.positions[baseIndex + 1] = particle.y;

      const colorIndex = count * 3;
      const sparkle = 0.7 + 0.3 * Math.sin(particle.twinkle + particle.age * 12);
      this.colors[colorIndex] = Math.min(1, particle.r * (brightener + sparkle * 0.6));
      this.colors[colorIndex + 1] = Math.min(1, particle.g * (brightener + sparkle * 0.6));
      this.colors[colorIndex + 2] = Math.min(1, particle.b * (brightener + sparkle * 0.6));

      this.sizes[count] = particle.baseSize * pixelRatio * (0.65 + particle.opacity * 0.45);
      this.opacities[count] = particle.opacity;

      count++;
    }

    return {
      positions: this.positions.subarray(0, count * 2),
      colors: this.colors.subarray(0, count * 3),
      sizes: this.sizes.subarray(0, count),
      opacities: this.opacities.subarray(0, count),
      count
    };
  }

  hslToRgb(h, s, l) {
    const hue = ((h % 1) + 1) % 1;
    if (s === 0) {
      return [l, l, l];
    }

    const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    const p = 2 * l - q;
    const r = this.hueToChannel(p, q, hue + 1 / 3);
    const g = this.hueToChannel(p, q, hue);
    const b = this.hueToChannel(p, q, hue - 1 / 3);
    return [r, g, b];
  }

  hueToChannel(p, q, t) {
    let temp = t;
    if (temp < 0) temp += 1;
    if (temp > 1) temp -= 1;
    if (temp < 1 / 6) return p + (q - p) * 6 * temp;
    if (temp < 1 / 2) return q;
    if (temp < 2 / 3) return p + (q - p) * (2 / 3 - temp) * 6;
    return p;
  }
}

class Renderer {
  constructor(canvas) {
    this.canvas = canvas;
    this.gl = canvas.getContext("webgl", { antialias: false, alpha: false });
    if (!this.gl) {
      throw new Error("WebGL is not supported in this browser.");
    }

    this.pixelRatio = window.devicePixelRatio || 1;

    this.program = this.createProgram();
    this.positionBuffer = this.gl.createBuffer();
    this.colorBuffer = this.gl.createBuffer();
    this.sizeBuffer = this.gl.createBuffer();
    this.opacityBuffer = this.gl.createBuffer();

    this.attribLocations = {
      position: this.gl.getAttribLocation(this.program, "a_position"),
      color: this.gl.getAttribLocation(this.program, "a_color"),
      size: this.gl.getAttribLocation(this.program, "a_size"),
      opacity: this.gl.getAttribLocation(this.program, "a_opacity")
    };

    this.uniformLocations = {
      projection: this.gl.getUniformLocation(this.program, "u_projection")
    };

    this.gl.enable(this.gl.BLEND);
    this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.ONE);
    this.gl.disable(this.gl.DEPTH_TEST);
    this.gl.clearColor(0.015, 0.018, 0.06, 1);

    this.resizeIfNeeded();
  }

  createShader(type, source) {
    const shader = this.gl.createShader(type);
    this.gl.shaderSource(shader, source);
    this.gl.compileShader(shader);
    if (!this.gl.getShaderParameter(shader, this.gl.COMPILE_STATUS)) {
      const info = this.gl.getShaderInfoLog(shader);
      this.gl.deleteShader(shader);
      throw new Error(`Shader compilation failed: ${info}`);
    }
    return shader;
  }

  createProgram() {
    const vertexSource = `
      attribute vec2 a_position;
      attribute vec3 a_color;
      attribute float a_size;
      attribute float a_opacity;

      uniform mat4 u_projection;

      varying vec3 v_color;
      varying float v_opacity;

      void main() {
        v_color = a_color;
        v_opacity = a_opacity;
        gl_Position = u_projection * vec4(a_position.xy, 0.0, 1.0);
        gl_PointSize = a_size;
      }
    `;

    const fragmentSource = `
      precision mediump float;

      varying vec3 v_color;
      varying float v_opacity;

      void main() {
        vec2 uv = gl_PointCoord * 2.0 - 1.0;
        float dist = dot(uv, uv);
        float alpha = smoothstep(1.0, 0.0, dist);
        float core = smoothstep(0.25, 0.0, dist);
        vec3 color = mix(v_color * 0.45, v_color, core);
        gl_FragColor = vec4(color, alpha * v_opacity);
      }
    `;

    const vertexShader = this.createShader(this.gl.VERTEX_SHADER, vertexSource);
    const fragmentShader = this.createShader(this.gl.FRAGMENT_SHADER, fragmentSource);
    const program = this.gl.createProgram();
    this.gl.attachShader(program, vertexShader);
    this.gl.attachShader(program, fragmentShader);
    this.gl.linkProgram(program);

    if (!this.gl.getProgramParameter(program, this.gl.LINK_STATUS)) {
      const info = this.gl.getProgramInfoLog(program);
      this.gl.deleteProgram(program);
      throw new Error(`Program linking failed: ${info}`);
    }

    return program;
  }

  resizeIfNeeded() {
    const ratio = window.devicePixelRatio || 1;
    const displayWidth = Math.floor(this.canvas.clientWidth * ratio) || this.canvas.width;
    const displayHeight = Math.floor(this.canvas.clientHeight * ratio) || this.canvas.height;
    const sizeChanged = displayWidth !== this.canvas.width || displayHeight !== this.canvas.height;
    const ratioChanged = Math.abs(ratio - this.pixelRatio) > 1e-3;

    if (sizeChanged) {
      this.canvas.width = displayWidth;
      this.canvas.height = displayHeight;
    }

    if (sizeChanged || ratioChanged) {
      this.pixelRatio = ratio;
      this.gl.viewport(0, 0, this.canvas.width, this.canvas.height);
      this.gl.useProgram(this.program);
      this.gl.uniformMatrix4fv(this.uniformLocations.projection, false, this.createProjectionMatrix());
    }
  }

  createProjectionMatrix() {
    return new Float32Array([
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, -1, 0,
      0, 0, 0, 1
    ]);
  }

  draw(drawData) {
    const { positions, colors, sizes, opacities, count } = drawData;
    const gl = this.gl;
    gl.clear(gl.COLOR_BUFFER_BIT);

    if (!count) {
      return;
    }

    gl.useProgram(this.program);

    gl.bindBuffer(gl.ARRAY_BUFFER, this.positionBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, positions, gl.DYNAMIC_DRAW);
    gl.enableVertexAttribArray(this.attribLocations.position);
    gl.vertexAttribPointer(this.attribLocations.position, 2, gl.FLOAT, false, 0, 0);

    gl.bindBuffer(gl.ARRAY_BUFFER, this.colorBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, colors, gl.DYNAMIC_DRAW);
    gl.enableVertexAttribArray(this.attribLocations.color);
    gl.vertexAttribPointer(this.attribLocations.color, 3, gl.FLOAT, false, 0, 0);

    gl.bindBuffer(gl.ARRAY_BUFFER, this.sizeBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, sizes, gl.DYNAMIC_DRAW);
    gl.enableVertexAttribArray(this.attribLocations.size);
    gl.vertexAttribPointer(this.attribLocations.size, 1, gl.FLOAT, false, 0, 0);

    gl.bindBuffer(gl.ARRAY_BUFFER, this.opacityBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, opacities, gl.DYNAMIC_DRAW);
    gl.enableVertexAttribArray(this.attribLocations.opacity);
    gl.vertexAttribPointer(this.attribLocations.opacity, 1, gl.FLOAT, false, 0, 0);

    gl.drawArrays(gl.POINTS, 0, count);
  }
}

class FountainVisualizer {
  constructor({ canvas, audioElement, fileInput, micButton, status }) {
    this.canvas = canvas;
    this.audioElement = audioElement;
    this.fileInput = fileInput;
    this.micButton = micButton;
    this.status = status;

    this.renderer = new Renderer(this.canvas);
    this.particles = new ParticleSystem(2048);

    this.audioContext = null;
    this.analyzer = null;
    this.mediaElementSource = null;
    this.microphoneSource = null;
    this.microphoneStream = null;
    this.fileUrl = null;

    this.lastTime = performance.now();

    this.handleFileSelection = this.handleFileSelection.bind(this);
    this.handleMicClick = this.handleMicClick.bind(this);
    this.animate = this.animate.bind(this);

    this.setupEventHandlers();
  }

  setupEventHandlers() {
    this.fileInput.addEventListener("change", this.handleFileSelection);
    this.micButton.addEventListener("click", this.handleMicClick);

    const resumeAudio = () => {
      if (this.audioContext && this.audioContext.state === "suspended") {
        this.audioContext.resume();
      }
    };

    document.addEventListener("pointerdown", resumeAudio);
    document.addEventListener("keydown", resumeAudio);
  }

  async ensureAudioContext() {
    if (!this.audioContext) {
      this.audioContext = new AudioContext();
      this.analyzer = new AudioAnalyzer(this.audioContext);
    }
    if (this.audioContext.state === "suspended") {
      await this.audioContext.resume();
    }
  }

  async handleFileSelection(event) {
    const files = event.target.files;
    if (!files || !files.length) {
      return;
    }

    const file = files[0];
    if (this.fileUrl) {
      URL.revokeObjectURL(this.fileUrl);
    }
    this.fileUrl = URL.createObjectURL(file);
    this.audioElement.src = this.fileUrl;

    await this.ensureAudioContext();
    this.detachMicrophone();
    this.attachMediaElement();

    try {
      await this.audioElement.play();
      this.updateStatus(`Playing: ${file.name}`);
    } catch (err) {
      this.updateStatus("Tap the play button to start audio.");
    }
  }

  async handleMicClick() {
    try {
      await this.ensureAudioContext();
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true, video: false });
      this.detachMediaElement();
      this.attachMicrophone(stream);
      this.updateStatus("Microphone connected.");
    } catch (err) {
      console.error(err);
      this.updateStatus("Microphone access was denied or unavailable.");
    }
  }

  attachMediaElement() {
    if (!this.audioContext || !this.analyzer) {
      return;
    }

    if (!this.mediaElementSource) {
      this.mediaElementSource = this.audioContext.createMediaElementSource(this.audioElement);
      this.mediaElementSource.connect(this.analyzer.input);
    }

    if (this.microphoneSource) {
      this.analyzer.disconnectSource(this.microphoneSource);
      this.microphoneSource = null;
    }

    if (this.microphoneStream) {
      this.microphoneStream.getTracks().forEach((track) => track.stop());
      this.microphoneStream = null;
    }

    this.analyzer.setOutputEnabled(true);
  }

  attachMicrophone(stream) {
    if (!this.audioContext || !this.analyzer) {
      return;
    }

    if (this.mediaElementSource) {
      this.analyzer.disconnectSource(this.mediaElementSource);
      this.mediaElementSource.disconnect();
      this.mediaElementSource = null;
    }

    this.audioElement.pause();
    this.audioElement.currentTime = 0;

    this.microphoneStream = stream;
    this.microphoneSource = this.audioContext.createMediaStreamSource(stream);
    this.microphoneSource.connect(this.analyzer.input);
    this.analyzer.setOutputEnabled(false);
  }

  detachMicrophone() {
    if (this.microphoneSource) {
      this.analyzer.disconnectSource(this.microphoneSource);
      this.microphoneSource.disconnect();
      this.microphoneSource = null;
    }
    if (this.microphoneStream) {
      this.microphoneStream.getTracks().forEach((track) => track.stop());
      this.microphoneStream = null;
    }
    this.analyzer?.setOutputEnabled(true);
  }

  updateStatus(message) {
    if (this.status) {
      this.status.textContent = message;
    }
  }

  start() {
    requestAnimationFrame(this.animate);
  }

  animate(now) {
    const delta = Math.min((now - this.lastTime) / 1000, 0.05);
    this.lastTime = now;

    const audioFrame = this.analyzer ? this.analyzer.update() : { energy: 0, peak: 0, bands: { low: 0, mid: 0, high: 0 } };
    this.renderer.resizeIfNeeded();
    const drawData = this.particles.update(delta, audioFrame, this.renderer.pixelRatio);
    this.renderer.draw(drawData);
    requestAnimationFrame(this.animate);
  }
}

function bootstrap() {
  const canvas = document.getElementById("glcanvas");
  const audioElement = document.getElementById("audio");
  const fileInput = document.getElementById("audio-file");
  const micButton = document.getElementById("mic-button");
  const status = document.getElementById("status");

  try {
    const visualizer = new FountainVisualizer({ canvas, audioElement, fileInput, micButton, status });
    visualizer.start();
    visualizer.updateStatus("Load an audio file or enable the microphone.");
  } catch (err) {
    console.error(err);
    status.textContent = "WebGL is not supported in this browser.";
  }
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", bootstrap);
} else {
  bootstrap();
}
