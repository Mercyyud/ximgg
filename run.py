import asyncio
import threading
import os
from dotenv import load_dotenv
from bot import bot, app
import uvicorn

load_dotenv()

def run_api():
    """Run the FastAPI server in a separate thread"""
    uvicorn.run(app, host="0.0.0.0", port=8000)

async def main():
    """Run the Discord bot"""
    await bot.start(os.getenv('TOKEN'))

if __name__ == "__main__":
    # Start the API server in a background thread
    api_thread = threading.Thread(target=run_api, daemon=True)
    api_thread.start()
    
    # Run the Discord bot
    asyncio.run(main())
