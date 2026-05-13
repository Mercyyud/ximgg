from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
import asyncpg
import os
from dotenv import load_dotenv

load_dotenv()

app = FastAPI(title="Xim.gg Auth API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/")
async def root():
    return {"status": "running", "service": "xim.gg auth api"}

@app.post("/api/log_login")
async def log_login(request: dict):
    guild_id = request.get("guild_id")
    license_key = request.get("license_key")
    hwid = request.get("hwid")
    
    if not guild_id or not license_key or not hwid:
        return {"status": "error", "message": "Missing required fields"}
    
    try:
        database_url = os.getenv('DATABASE_URL')
        conn = await asyncpg.connect(database_url)
        
        result = await conn.fetchrow(
            'SELECT id, hwid, is_active FROM license_keys WHERE key = $1',
            license_key
        )
        await conn.close()
        
        if not result:
            return {"status": "error", "message": "Invalid license key"}
        if not result['is_active']:
            return {"status": "error", "message": "License key is inactive"}
        
        # Update last_used and bind HWID
        conn = await asyncpg.connect(database_url)
        await conn.execute(
            'UPDATE license_keys SET hwid = $1, last_used = CURRENT_TIMESTAMP WHERE key = $2',
            hwid, license_key
        )
        await conn.close()
        
        return {"status": "success"}
    except Exception as e:
        return {"status": "error", "message": str(e)}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
