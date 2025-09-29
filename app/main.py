from fastapi import FastAPI, Header, HTTPException, Form
from fastapi.responses import Response, JSONResponse
from pydantic import BaseModel
import base64, io, os
from PIL import Image
from rembg import remove

API_KEY = os.getenv("BG_API_KEY", "")

app = FastAPI()

@app.get("/health")
def health():
    return {"status": "ok"}

@app.post("/removebg")
def remove_bg(
    image_file_b64: str = Form(...),
    size: str = Form("auto"),
    x_api_key: str | None = Header(default=None, alias="X-Api-Key")
):
    # Auth
    if not API_KEY or x_api_key != API_KEY:
        raise HTTPException(status_code=401, detail="invalid or missing API key")

    # Decode image
    try:
        raw = base64.b64decode(image_file_b64)
        img = Image.open(io.BytesIO(raw))
        # enforce RGB for JPEG inputs; preserves original resolution
        if img.mode not in ("RGB", "RGBA"):
            img = img.convert("RGB")
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"invalid image data: {e}")

    # (Optional) size handling – we ignore 'auto' to preserve your 4K dimensions.
    # You could downsample here if you ever want faster inference.

    try:
        # rembg returns raw PNG bytes with alpha
        out_bytes = remove(raw)  # operates on original size
        return Response(content=out_bytes, media_type="image/png")
    except Exception as e:
        return JSONResponse(
            status_code=422,
            content={"error": "could not identify foreground", "detail": str(e)}
        )
