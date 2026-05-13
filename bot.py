import discord
from discord import app_commands
from discord.ext import commands
import os
import json
import aiofiles
import asyncpg
from dotenv import load_dotenv
from datetime import datetime
import logging
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
import uvicorn

# Setup logging to file
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(name)s: %(message)s',
    handlers=[
        logging.FileHandler('bot.log', encoding='utf-8'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger('discord')

load_dotenv()

class TicketBot(commands.Bot):
    def __init__(self):
        intents = discord.Intents.default()
        intents.message_content = True
        intents.members = True
        
        super().__init__(
            command_prefix="!",
            intents=intents,
            help_command=None
        )
        
    async def setup_hook(self):
        await self.init_database()
        await self.load_cogs()
        atabase_url = os.getenv('DATABASE_URL')
        if database_url:
            # Use Railway PostgreSQL
            self.db = await asyncpg.create_pool(database_url)
            await self.db.execute('''
                CREATE TABLE IF NOT EXISTS ticket_configs (
                    guild_id BIGINT PRIMARY KEY,
                    ticket_category BIGINT,
                    transcript_channel BIGINT,
                    support_role BIGINT,
                    ticket_counter INTEGER DEFAULT 0,
                    log_channel_minor BIGINT,
                    log_channel_major BIGINT
                )
            ''')
            await self.d.execute('''
                CREATE TABLE IF NOT EXISTS ticketcategories (
                    id SERIAL PRIMARY KEY,
                    guild_id BIGINT,
                    name TEXT,
                    descrition TEXT,
                    emoji TEXT,
                    role_id BIGINT,
                    category_id BIGINT
                )
            ''')
            await self.db.execute('''
                CREATE TABLE IF NOT EXISTS tickes (
                    cannel_id BIGINT PRIMARY KEY,
                    guild_id BIGINT,
                   user_id BIGINT,
                    category TEXT,
                    created_at TIMESTAMP,
                    closed_at TIMESTAMP,
                    closed_by BIGINT
                )
            ''')
            await self.db.execute('''
                CREATE TABLE IF NOT EXISTS welcome_configs (
                    guild_id BIGINT PRIMARY KEY,
                    channel_id BIGINT,
                    enabled BOOLEAN DEFAULT true
                )
            ''')
            await self.db.execute('''
                CREATE TABLE IF NOT EXISTS license_keys (
                    id SERIAL PRIMARY KEY,
                    key TEXT UNIQUE NOT NULL,
                    hwid TEXT,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    last_used TIMESTAMP,
                    is_active BOOLEAN DEFAULT true
                )
            ''')
        else:
            # Fallback to SQLite
            import aiosqlite
            db_path = os.getenv('DB_PATH', 'tickets.db')
            self.db = await aiosqlite.connect(db_path)
            await self.db.execute('''
                CREATE TABLE IF NOT EXISTS ticket_configs (
                    guild_id INTEGER PRIMARY KEY,
                    ticket_category INTEGER,
                    transcript_channel INTEGER,
                    support_role INTEGER,
                    ticket_counter INTEGER DEFAULT 0,
                    log_channel_minor INTEGER,
                    log_channel_major INTEGER
                )
            ''')
            await self.db.execute('''
                CREATE TABLE IF NOT EXISTS ticket_categories (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    guild_id INTEGER,
                    name TEXT,
                    description TEXT,
                    emoji TEXT,
                    role_id INTEGER,
                    category_id INTEGER
                )
            ''')
            await self.db.execute('''
                CREATE TABLE IF NOT EXISTS tickets (
                    channel_id INTEGER PRIMARY KEY,
                    guild_id INTEGER,
                    user_id INTEGER,
                    category TEXT,
                    created_at TIMESTAMP,
                    closed_at TIMESTAMP,
                    closed_by INTEGER
                )
            ''')
            await self.db.execute('''
                CREATE TABLE IF NOT EXISTS welcome_configs (
                    guild_id INTEGER PRIMARY KEY,
                    channel_id INTEGER,
                    enabled INTEGER DEFAULT 1
                )
            ''')
            await self.db.execute('''
                CREATE TABLE IF NOT EXISTS license_keys (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    key TEXT UNIQUE NOT NULL,
                    hwid TEXT,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    last_used TIMESTAMP,
                    is_active INTEGER DEFAULT 1
                )
            ''')
            await self.db.commit()
        
    async def load_cogs(self):
        await self.load_extension('cogs.tickets')
        await self.load_extension('cogs.admin')
        await self.load_extension('cogs.welcome')
        await self.load_extension('cogs.logging_cog')
        await self.load_extension('cogs.modmail')
        
    async def on_ready(self):
        await self.tree.sync()
        logger.info(f'Bot logged in as {self.user} (ID: {self.user.id})')
        logger.info(f'Invite link: https://discord.com/api/oauth2/authorize?client_id={self.user.id}&permissions=8&scope=bot%20applications.commands')
        print(f'🤖 Logged in as {self.user} (ID: {self.user.id})')
        print(f'🔗 Invite: https://discord.com/api/oauth2/authorize?client_id={self.user.id}&permissions=8&scope=bot%20applications.commands')
    
    async def on_command_completion(self, ctx):
        logger.info(f'Command used: {ctx.command.name} by {ctx.author} in {ctx.guild.name}')
    
    async def on_app_command_completion(self, interaction, command):
        logger.info(f'Slash command used: {command.name} by {interaction.user} in {interaction.guild.name}')
    
    async def on_error(self, event_method, *args, **kwargs):
        logger.error(f'Error in {event_method}: {args} {kwargs}', exc_info=True)

bot = TicketBot()

@bot.event
async def on_close():
    await bot.db.close()

# FastAPI app for auth endpoint
app = FastAPI(title="Xim.gg Auth API")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.post("/api/log_login")
async def log_login(request: dict):
    guild_id = request.get("guild_id")
    license_key = request.get("license_key")
    hwid = request.get("hwid")
    
    if not guild_id or not license_key or not hwid:
        return {"status": "error", "message": "Missing required fields"}
    
    try:
        # Check if key exists and is active
        if isinstance(bot.db, asyncpg.Pool):
            # PostgreSQL
            result = await bot.db.fetchrow(
                'SELECT id, hwid, is_active FROM license_keys WHERE key = $1',
                license_key
            )
            if not result:
                return {"status": "error", "message": "Invalid license key"}
            if not result['is_active']:
                return {"status": "error", "message": "License key is inactive"}
            
            # Update last_used and bind HWID if not bound
            await bot.db.execute(
                'UPDATE license_keys SET hwid = $1, last_used = CURRENT_TIMESTAMP WHERE key = $2',
                hwid, license_key
            )
            return {"status": "success"}
        else:
            # SQLite
            async with bot.db.execute(
                'SELECT id, hwid, is_active FROM license_keys WHERE key = ?',
                (license_key,)
            ) as cursor:
                row = await cursor.fetchone()
                if not row:
                    return {"status": "error", "message": "Invalid license key"}
                if not row[3]:  # is_active
                    return {"status": "error", "message": "License key is inactive"}
            
            await bot.db.execute(
                'UPDATE license_keys SET hwid = ?, last_used = CURRENT_TIMESTAMP WHERE key = ?',
                (hwid, license_key)
            )
            await bot.db.commit()
            return {"status": "success"}
    except Exception as e:
        return {"status": "error", "message": str(e)}

if __name__ == "__main__":
    import asyncio
    
    async def main():
        await bot.start(os.getenv('TOKEN'))
    
    # Run both Discord bot and FastAPI server
    import threading
    def run_api():
        uvicorn.run(app, host="0.0.0.0", port=8000)
    
    api_thread = threading.Thread(target=run_api, daemon=True)
    api_thread.start()
    
    asyncio.run(main())
