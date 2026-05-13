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
    token = os.getenv('TOKEN')
    if not token:
        print("WARNING: DISCORD_TOKEN not set. Running API server only.")
        return
    await bot.start(token)

if __name__ == "__main__":
    # Start the API server in a background thread
    api_thread = threading.Thread(target=run_api, daemon=True)
    api_thread.start()
    
    # Run the Discord bot if token is available
    asyncio.run(main())
    
    # Keep the main thread alive if only API is running
    if not os.getenv('TOKEN'):
        api_thread.join()
