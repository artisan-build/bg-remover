# bg-remover
The replacement repository to take on the task of background removal for images, emulating remove.bg but acting on our own environment.  Below is the current approach, which will be replaced by this functionality.
### How remove.bg integrates with the highlight‑video pipeline

1.  Portrait upload & preprocessing

    -   When a camper uploads a portrait, the `PlayerPortraitUploader` component stores the file and sequentially runs several actions: converting non‑JPEG formats, scaling the image to 4K, invoking the background‑removal step, and producing a smaller preview.

    -   The resulting files---including the 4K image without a background (`player‑2160p‑nobg.png`)---are then persisted to remote object storage for later steps in the build process.

2.  Background removal via remove.bg

    -   `RemoveBackgroundFromPortrait` checks for the scaled 4K portrait (`player‑2160p.jpg`) and, unless running in mock mode, sends it to remove.bg.

    -   The request includes:

        -   Header `X-Api-Key` (configured as `services.background_remover.key`)

        -   Body parameters\
            `image_file_b64` -- the base64‑encoded JPEG\
            `size` -- `'auto'`

    -   A PNG with the background removed is returned and saved locally as `player‑2160p‑nobg.png`.

3.  Use in the intro video

    -   During highlight‑video generation, `PlayerIntroStepBuilder` inserts `RemoveBackgroundFromPortrait` between scaling and preview steps, ensuring all downstream actions operate on the isolated portrait.

    -   The `MergeAthletePhotoToVideo` action overlays this transparent PNG onto a motion template, creating the personalized intro segment for the final highlight video.

### Full data flow summary

1.  User portrait → saved locally.

2.  Non‑JPEG formats converted; image scaled to 4K.

3.  4K image sent to remove.bg → PNG without background returned.

4.  PNG stored locally and as smaller preview; both pushed to remote storage.

5.  During video build, stored assets are pulled, the transparent PNG is composited into the intro video, and the final highlight video is compiled.

Through this pipeline, remove.bg acts as an external service that receives a base64‑encoded 4K portrait and returns a foreground‑isolated PNG, which is then used to personalize the athlete's highlight video.

Describe the overall highlight video tool.

Copy

Overview of the Highlight Video Tool
------------------------------------

The highlight video tool lets campers transform raw game footage from EXACT camps into a polished highlight reel through an interactive multi‑step workflow and a background job pipeline.

### Camper‑Facing Workflow

1.  Launch Builder\
    `BuildHighlightVideo` renders a landing page where campers can begin or resume creating a video for their sport.

2.  Wizard Steps

    -   About You -- Collects personal details (name, team, position, etc.), handles portrait upload, and triggers the intro‑video build when the photo is validated and background-removed.

    -   Find Highlights -- Allows campers to browse camp recordings, choose in/out points for each clip, and tag themselves by position/size on the frame. Each saved highlight dispatches its own processing job.

    -   Sort Clips -- Drag‑and‑drop ordering of highlights to define final playback sequence.

    -   Select Music -- Placeholder for choosing background music (currently marked "in development").

    -   Finish Video -- Submits the compiled video for rendering, shows progress/status, and offers retry on failure.

### Rendering Pipeline

1.  GenerateHighlightVideo (Job)

    -   Orchestrates the entire rendering process.

    -   Checks remote storage (Cloudflare R2) for locks/hashes to avoid redundant work.

    -   Dispatches:

        -   `BuildPlayerIntro` for the personalized intro segment.

        -   `BuildHighlight` for each selected clip.

        -   `BuildVideo` when all components are ready.

2.  BuildPlayerIntro / BuildHighlight / BuildVideo

    -   Use `ActionRunner` with step builders:

        -   `PlayerIntroStepBuilder` handles portrait processing (scale, remove.bg background removal, merge into template, add text, generate preview/OG image, save to R2).

        -   `HighlightStepBuilder` extracts clip from source video, generates overlay mask, constructs highlight clip, saves artifacts, and unlocks/locks as needed.

        -   `StepListBuilder` runs global steps (copy assets, compile final video, generate preview, upload finished files).

    -   Each action logs progress, relies on hashes to skip unchanged steps, and writes logs/lock files to R2 for coordination.

    -   Upon success, `BuildVideo` saves the final MP4 to public storage, updates the database, and emails the camper (different templates for paid vs unpaid).

### Storage & Status Management

-   ScopedDiskBuilder provides local, remote, and public filesystems tied to a video's UUID.

-   HighlightVideoRepository wraps operations for a single video: listing files, dispatching render/preview jobs, checking flags (queued/processing/rendered/failed), and generating public URLs.

-   Log and lock files under `logs/` and `intro.lock`/`clips/{uuid}.lock` ensure safe concurrent processing and help recover from failures.

### Admin & Supporting Features

-   Livewire components and Filament resources expose dashboards to monitor highlight videos, view failure modes, and interact with the generation process.

-   Notification classes send emails when videos are ready or require user intervention.

-   Commands (e.g., `highlight-video:generate`, `publish`, `pull`, etc.) and tests support maintenance, deployment, and validation.

Together, these components create a robust system for campers to curate highlight clips, personalize their intro, and receive a professionally rendered video with minimal manual intervention.

If replacing remove.bg with another tool for removing backgrounds, what requirements should it meet to effectively interface with this laravel application?

Copy

To swap out remove.bg you need a background‑removal service (or library) that can be called from the `RemoveBackgroundFromPortrait` action and still satisfy the expectations of the rest of the highlight‑video pipeline.

### Functional requirements

-   Input format -- Accept a 4K portrait (`player-2160p.jpg`) encoded in base64.

-   Output format -- Return a PNG with transparency (saved as `player-2160p-nobg.png`) at the same resolution.

-   Synchronous call -- Process the image and return the result in a single HTTP response (no polling/async workflow).

-   Reliability & resolution -- Must handle 4K images and remove backgrounds accurately; failure should be indicated via HTTP status.

### API & configuration requirements

-   HTTP interface -- Exposed over HTTPS so Laravel's `Http` facade can `POST` to it.

-   Authentication -- Accept an API key or similar credential via request headers; key stored in `config/services.php` (`background_remover.key`).

-   Parameters -- Support a "size" or equivalent parameter; the current action sends `size => 'auto'`.

-   Error messages -- Provide descriptive failure messages (e.g., "could not identify foreground") so the UI can display a meaningful error.

### Integration expectations

-   Drop‑in action -- The `RemoveBackgroundFromPortrait` action should still only need to supply the portrait file, the API key, and parse the returned PNG.

-   Mocking/testing -- Allow skipping external calls (the action has a `$mock` flag) so tests can inject prebuilt images.

A tool meeting these requirements can be wired into the existing action without disrupting the portrait upload workflow or the intro‑video build process.

What is needed to be done to complete below (ignoring optional elements)? ** A tiny Python **compatibility shim** that emulates your current remove.bg call (same header & body), but uses Rembg under the hood. This lets you change only the **endpoint URL** in Laravel. I'll outline Path A in full (infra + code + Laravel changes), then note Path B if you want to shortcut. --- # "Drop-in" Rembg service that mimics remove.bg ## What you'll have when done * A service at `https://bg.example.com/removebg` that: * accepts `X-Api-Key` header, * accepts `image_file_b64` and optional `size` (ignored or mapped), * returns a **transparent PNG** (same resolution) in the HTTP response body, * returns JSON errors with useful messages and proper status codes. Your Laravel code keeps its request shape; you'll just point to a new URL and rotate the key. --- ## 1) Make a small repo for the service Create a new GitHub repo (e.g., `exactsports/bg-remover`) with these files: **`pyproject.toml`** ```toml [project] name = "bg-remover" version = "0.1.0" requires-python = ">=3.10" dependencies = [ "fastapi", "uvicorn[standard]", "pillow", "rembg", # CPU build; see GPU note below "python-multipart", ] [tool.uvicorn] host = "0.0.0.0" port = 7000 ``` **`app/main.py`** ```python from fastapi import FastAPI, Header, HTTPException, Form from fastapi.responses import Response, JSONResponse from pydantic import BaseModel import base64, io, os from PIL import Image from rembg import remove API_KEY = os.getenv("BG_API_KEY", "") app = FastAPI() @app.get("/health") def health(): return {"status": "ok"} @app.post("/removebg") def remove_bg( image_file_b64: str = Form(...), size: str = Form("auto"), x_api_key: str | None = Header(default=None, alias="X-Api-Key") ): # Auth if not API_KEY or x_api_key != API_KEY: raise HTTPException(status_code=401, detail="invalid or missing API key") # Decode image try: raw = base64.b64decode(image_file_b64) img = Image.open(io.BytesIO(raw)) # enforce RGB for JPEG inputs; preserves original resolution if img.mode not in ("RGB", "RGBA"): img = img.convert("RGB") except Exception as e: raise HTTPException(status_code=400, detail=f"invalid image data: {e}") # (Optional) size handling -- we ignore 'auto' to preserve your 4K dimensions. # You could downsample here if you ever want faster inference. try: # rembg returns raw PNG bytes with alpha out_bytes = remove(raw) # operates on original size return Response(content=out_bytes, media_type="image/png") except Exception as e: return JSONResponse( status_code=422, content={"error": "could not identify foreground", "detail": str(e)} ) ``` **`Dockerfile`** ```dockerfile FROM python:3.11-slim # System deps for pillow (libjpeg, zlib) RUN apt-get update && apt-get install -y --no-install-recommends \ libjpeg62-turbo-dev zlib1g-dev && \ rm -rf /var/lib/apt/lists/* WORKDIR /app COPY pyproject.toml /app/ RUN pip install --no-cache-dir uv pip setuptools wheel && \ pip install --no-cache-dir -r <(python -c "import tomllib,sys;print('\n'.join(__import__('tomllib').loads(open('pyproject.toml','rb').read())['project']['dependencies']))") COPY app /app/app ENV PYTHONUNBUFFERED=1 EXPOSE 7000 CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "7000"] ``` **`compose.yaml`** ```yaml services: bg-remover: build: . image: exactsports/bg-remover:latest env_file: .env ports: - "7000:7000" restart: unless-stopped # GPU note: if you add a GPU build (see below), uncomment: # deploy: # resources: # reservations: # devices: # - capabilities: [gpu] ``` **`.env.example`** ``` BG_API_KEY=change-me ``` > **GPU optional:** For higher throughput at 4K, you can switch to a GPU image and install a CUDA-enabled `torch` + `rembg` stack. That's an extra step (pin PyTorch to your server's CUDA), so start CPU and scale up only if needed. --- ## 2) Deploy the service on Forge **Option 1 --- Dedicated server (cleanest):** 1. Provision a small Ubuntu server in Forge (start with 2--4 vCPU, 8--16GB RAM; add GPU later if needed). 2. SSH into the server once (Forge will give you the connection). 3. Install Docker & Docker Compose plugin (Forge's standard steps or your own bootstrap). 4. In Forge, under **Sites**, create a new **Static** site for `bg.example.com`. 5. Under that site: * **Deploy Script:** `git clone` your `bg-remover` repo into `/home/forge/bg.example.com`, then `docker compose up -d --build`. * **Daemons (optional):** Add a daemon that runs `docker compose up` to self-heal on reboot. 6. **Nginx proxy:** Edit the site's Nginx config in Forge to reverse-proxy `bg.example.com` to `127.0.0.1:7000`, and issue an SSL cert (Let's Encrypt). * Forge's Static site already points to Nginx; just add: ``` location / { proxy_pass http://127.0.0.1:7000; proxy_set_header Host $host; proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for; proxy_set_header X-Forwarded-Proto $scheme; proxy_read_timeout 300s; } ``` 7. Set the environment variable in Forge → **Environment** tab for the site: * `BG_API_KEY=<long-random-string>` 8. Click **Deploy Now**. **Option 2 --- Same server as Laravel (works, but isolate with care):** * Repeat the above steps on your existing app server and choose a subdomain like `bg.internal.example.com`. Be mindful of CPU/RAM contention; large 4K jobs can spike memory usage. **Health & monitoring** * Forge **Heartbeat**: hit `https://bg.example.com/health` every minute; alert on failure. * Add an Nginx **rate limit** (e.g., 5 req/s) to protect against bursts. * Optional: restrict IP to your app servers or put it behind Cloudflare Zero Trust. --- ## 3) Wire Laravel to the new service (no code shape changes) **.env** ``` BACKGROUND_REMOVER_URL="https://bg.example.com/removebg" BACKGROUND_REMOVER_KEY="same-as-BG_API_KEY" BACKGROUND_REMOVER_PROVIDER="rembg_local" # for feature flags/telemetry ``` **config/services.php** ```php 'background_remover' => [ 'url' => env('BACKGROUND_REMOVER_URL'), 'key' => env('BACKGROUND_REMOVER_KEY'), 'provider' => env('BACKGROUND_REMOVER_PROVIDER', 'rembg_local'), 'timeout' => 60, // seconds; adjust for 4K ], ``` **app/Actions/RemoveBackgroundFromPortrait.php** (key excerpt) ```php use Illuminate\Support\Facades\Http; use Illuminate\Support\Str; public function handle(string $jpegPath, bool $mock = false): string { $pngOut = Str::replaceLast('.jpg', '-nobg.png', $jpegPath); if ($mock) { // copy a fixture for tests copy(base_path('tests/fixtures/player-2160p-nobg.png'), $pngOut); return $pngOut; } $bytes = file_get_contents($jpegPath); $b64 = base64_encode($bytes); $resp = Http::timeout(config('services.background_remover.timeout')) ->withHeaders([ 'X-Api-Key' => config('services.background_remover.key'), ]) ->asForm() // our shim expects form fields ->post(config('services.background_remover.url'), [ 'image_file_b64' => $b64, 'size' => 'auto', // accepted but ignored in the shim (preserves 4K) ]); if ($resp->failed()) { $msg = $resp->json('error') ?? $resp->body(); throw new \RuntimeException("Background removal failed: {$msg}"); } file_put_contents($pngOut, $resp->body()); return $pngOut; } ``` > This preserves your existing action's call pattern and return behavior. Your **PlayerIntroStepBuilder** and **MergeAthletePhotoToVideo** can remain unchanged. --- ## 4) Testing & rollout **Local dev** * Run the service locally: ``` docker compose up --build curl https://localhost:7000/health ``` * From Laravel, run a unit test that sets `$mock=false` against `http://localhost:7000/removebg` and asserts a PNG with alpha is returned and the output dimensions match the input. **Staging canary** * Add a quick feature flag in `.env`: ``` BACKGROUND_REMOVER_ENABLED=true ``` * Wrap the call; if disabled, fall back to remove.bg for comparison for a few days. * Log per-image latency and success rate to your usual logger (or Sentry), with tags: * `provider=rembg_local`, `resolution=2160p`, `duration_ms`, `result=ok|error`. **Quality check** * Run a set of 50--100 real camper portraits through both remove.bg (current) and rembg shim. * Have an ops/creative pass/fail rubric (hair detail, jersey edges, artifact count). * If a subset fails consistently (e.g., busy stadium backgrounds), consider: * Switching Rembg model (e.g., human\_seg), * Adding a pre-blur for background noise, * Or piloting MODNet later just for those edge cases. --- ## 5) Performance & scaling * **CPU first:** For sporadic 4K jobs, modern 4 vCPU + 8--16 GB RAM hosts are fine. Expect 1--3s per image; parallelize via queue if needed. * **GPU later (optional):** If you process bursts of 4K portraits (e.g., camp spikes), add a GPU server and install a CUDA-enabled PyTorch + `rembg`. In Docker, use `--gpus all` and a torch build that matches the host driver. You'll likely see 3--10× throughput improvement. * **Timeouts & limits:** Keep the Laravel request timeout at 60--120s; cap request size in Nginx (e.g., `client_max_body_size 30m`); add a concurrency cap via a simple worker queue or Nginx rate-limit zone. --- ## 6) Security & ops * **Secrets:** Keep `BG_API_KEY` only in Forge env vars (not in Git). * **Network:** Prefer a private service (VPC / allowlist app server IPs). If public, keep Cloudflare in front and enable WAF rules. * **Observability:** Add `/health` to Forge Heartbeats; tail container logs; optionally expose Prometheus metrics if you want deeper insights (requests, p95, failures).

Copy

To complete the drop‑in Rembg service (ignoring optional GPU, monitoring, etc.):

1.  Create the Python service repo

    -   Add `pyproject.toml`, `app/main.py`, `Dockerfile`, `compose.yaml`, and `.env.example` exactly as specified.

    -   Ensure `/removebg` endpoint accepts `X-Api-Key` and `image_file_b64`, returns PNG, and reports errors in JSON.

2.  Build & run the container

    -   `docker compose up --build` locally to verify `/health` and `/removebg` endpoints.

3.  Provision & deploy on a server (Forge or similar)

    -   Install Docker + Compose.

    -   Clone repo and run `docker compose up -d` in deployment script.

    -   Set `BG_API_KEY` env var.

    -   Configure Nginx to proxy `https://bg.example.com` → `127.0.0.1:7000` and issue TLS cert.

4.  Integrate with Laravel

    -   Update `.env`:\
        `BACKGROUND_REMOVER_URL`, `BACKGROUND_REMOVER_KEY`, `BACKGROUND_REMOVER_PROVIDER`.

    -   Expand `config/services.php` to include URL, key, provider, timeout.

    -   Modify `RemoveBackgroundFromPortrait` action to call the new URL (form data, same header) and save returned PNG.

5.  Verify

    -   Run the portrait‑upload or a unit test with `$mock=false` and ensure a transparent PNG is produced at the same resolution.

    -   Deploy and confirm real portraits render correctly through the new service.

These steps deliver a functional Rembg-based background remover that mimics remove.bg with only the endpoint and key changed in Laravel.

Will this be done in a separate repository or within the /camps repo?

Copy

The background‑removal shim is intended as its own small repository (e.g., `bg-remover`) running a Python FastAPI service. The existing `/camps` Laravel repo stays separate; you only adjust environment variables and configuration there to point at the new service.

Once we make the above changes in the new bg-remover application, what needs updating in this camps repository?

Copy

### Changes required in the `camps` repository

1.  Environment variables

    -   Add new values in `.env` (and relevant deployment secrets):

        -   `BACKGROUND_REMOVER_URL="https://bg.example.com/removebg"`

        -   `BACKGROUND_REMOVER_KEY="<same BG_API_KEY used by the service>"`

        -   `BACKGROUND_REMOVER_PROVIDER="rembg_local"` (or similar feature‑flag).

2.  Service configuration

    -   Update `config/services.php` to include a `background_remover` section:

        `'background_remover' => [ 'url' => env('BACKGROUND_REMOVER_URL'), 'key' => env('BACKGROUND_REMOVER_KEY'), 'provider' => env('BACKGROUND_REMOVER_PROVIDER', 'rembg_local'), 'timeout' => 60, // adjust if needed ], `

3.  Background‑removal action

    -   Ensure `app/Actions/RemoveBackgroundFromPortrait.php` reads the new config and posts the portrait to the shim:

        -   Include header `X-Api-Key`.

        -   Send form fields `image_file_b64` and `size => 'auto'`.

        -   Write the returned PNG body to `player-2160p-nobg.png`.

        -   Surface JSON error messages on failure.

4.  Testing / configuration validation

    -   Adjust any tests or local environment settings that call `RemoveBackgroundFromPortrait` to point at the new service (e.g., by setting the env vars in testing config).

    -   Verify a portrait upload produces a transparent PNG at the same resolution using the shim.

With these updates, the `camps` app will target the new FastAPI-based background remover without further changes to the rest of the highlight-video pipeline.
