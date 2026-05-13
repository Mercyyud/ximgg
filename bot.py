import discord
from discord import app_commands
from fastapi import FastAPI, Request, HTTPException
import psycopg2
from psycopg2 import pool
from datetime import datetime, timedelta
import asyncio
import uvicorn
import logging
import io
import re
import os
import time
import secrets
import string
import json
from typing import Optional
from functools import wraps
from concurrent.futures import ThreadPoolExecutor

# Thread pool for blocking database operations
db_executor = ThreadPoolExecutor(max_workers=10)
from concurrent.futures import ThreadPoolExecutor
# Thread pool for blocking database operations
db_executor = ThreadPoolExecutor(max_workers=20)

# --- CONFIGURATION ---
MASTER_DB_URL = os.getenv("MASTER_DB_URL", "")
BOT_TOKEN = os.getenv("BOT_TOKEN", "")
SEBWETT_ID = int(os.getenv("SEBWETT_ID", "983407797972656129"))
OLD_GUILD_ID = int(os.getenv("OLD_GUILD_ID", "1419533443812950088"))
PORT = int(os.getenv("PORT", "8000"))

logging.basicConfig(level=logging.INFO, format="%(asctime)s | %(levelname)s | %(message)s")
logger = logging.getLogger("SystemBot")

client = discord.Client(intents=discord.Intents.all())
tree = app_commands.CommandTree(client)
app = FastAPI()  

# --- DATABASE POOLING ---
db_pools = {}  # Guild ID -> connection pool

def get_table_prefix(guild_id: str):
    """Get table prefix for a guild (handles linked servers)"""
    try:
        # Check if this server is linked to another server
        m_conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=5)
        m_cur = m_conn.cursor()
        m_cur.execute("SELECT linked_from_guild_id FROM server_configs WHERE guild_id = %s", (guild_id,))
        result = m_cur.fetchone()
        m_conn.close()
        
        if result and result[0]:
            # This server is linked, use the original server's table prefix
            return f"server_{result[0]}"
        else:
            # This server is not linked, use its own table prefix
            return f"server_{guild_id}"
    except Exception:
        # Fallback to default behavior if there's any error
        return f"server_{guild_id}"

def get_pool(guild_id: str):
    """Get or create connection pool for a guild"""
    if guild_id not in db_pools:
        try:
            m_conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=5)
            m_cur = m_conn.cursor()
            m_cur.execute("SELECT db_url FROM server_configs WHERE guild_id = %s", (guild_id,))
            res = m_cur.fetchone()
            m_conn.close()
            
            if res:
                db_url = res[0]
                if db_url.startswith('postgresql://') or db_url.startswith('postgres://'):
                    # Optimized pool settings for faster connections
                    db_pools[guild_id] = psycopg2.pool.SimpleConnectionPool(
                        5, 20, db_url, connect_timeout=5
                    )
        except Exception as e:
            logger.error(f"Failed to create pool for guild {guild_id}: {e}")
    
    return db_pools.get(guild_id)

def get_db_connection(guild_id: str, max_retries: int = 3):
    """Get fast database connection from pool with retry mechanism"""
    for attempt in range(max_retries):
        pool = get_pool(str(guild_id))
        if pool:
            try:
                conn = pool.getconn()
                # Test the connection to ensure it's valid
                cur = conn.cursor()
                cur.execute("SELECT 1")
                cur.close()
                return conn
            except psycopg2.pool.PoolError as e:
                logger.warning(f"Pool exhausted for guild {guild_id}, attempt {attempt + 1}/{max_retries}: {e}")
                if attempt < max_retries - 1:
                    asyncio.sleep(0.1 * (attempt + 1))  # Brief delay before retry
                    continue
            except Exception as e:
                logger.error(f"Connection error for guild {guild_id}, attempt {attempt + 1}/{max_retries}: {e}")
                if conn:
                    try:
                        pool.putconn(conn, close=True)  # Close bad connection
                    except:
                        pass
                if attempt < max_retries - 1:
                    continue
        else:
            logger.warning(f"No pool available for guild {guild_id}, attempt {attempt + 1}/{max_retries}")
            if attempt < max_retries - 1:
                asyncio.sleep(0.1)
                continue
    return None

def release_db_connection(guild_id: str, conn):
    """Release connection back to pool"""
    pool = get_pool(str(guild_id))
    if pool and conn:
        try:
            pool.putconn(conn)
        except:
            pass

# --- SECURITY & VALIDATION ---

def requires_config(func):
    """Decorator to check if server is configured before running command"""
    async def wrapper(interaction: discord.Interaction, *args, **kwargs):
        # Check if server is configured
        conn = get_db_connection(interaction.guild_id)
        if not conn:
            emb = create_not_configured_embed(interaction.guild_id)
            return await interaction.response.send_message(embed=emb, ephemeral=True)
        
        # Release connection and continue with command
        release_db_connection(str(interaction.guild_id), conn)
        return await func(interaction, *args, **kwargs)
    
    return wrapper

def create_not_configured_embed(guild_id: int):
    """Create a simple 'not configured' message"""
    return "🚀 This server needs some love! 💫\n\nRun /setup to unlock full power! 🎯"

def requires_setup_permission(func):
    """Decorator to allow only setup command on unconfigured servers"""
    async def wrapper(interaction: discord.Interaction, *args, **kwargs):
        # Check if user is system owner (always allowed for setup)
        if interaction.user.id != SEBWETT_ID:
            emb = create_modern_embed("Access Denied", guild_id=interaction.guild_id)
            emb.description = "Only the system owner can run `/setup` on unconfigured servers."
            return await interaction.response.send_message(embed=emb, ephemeral=True)
        
        # Check if server is NOT configured (setup only works on unconfigured servers)
        conn = get_db_connection(interaction.guild_id)
        if conn:
            release_db_connection(str(interaction.guild_id), conn)
            emb = create_modern_embed("Already Configured", guild_id=interaction.guild_id)
            emb.description = "This server is already configured! Use other commands for management."
            return await interaction.response.send_message(embed=emb, ephemeral=True)
        
        return await func(interaction, *args, **kwargs)
    
    return wrapper

def validate_input(input_str: str, max_length: int = 1000) -> str:
    """Sanitize and validate user input"""
    if not input_str or len(input_str) > max_length:
        return ""
    # Remove potentially dangerous characters
    cleaned = re.sub(r'[<>"\'\x00-\x1f\x7f-\x9f]', '', input_str)
    return cleaned.strip()

def validate_days(days: int) -> bool:
    """Validate days parameter"""
    return isinstance(days, int) and 1 <= days <= 3650  # Max 10 years

def validate_user_id(user_id: int) -> bool:
    """Validate Discord user ID"""
    return isinstance(user_id, int) and 100000000000000000 <= user_id <= 9223372036854775807

def generate_license_key(custom_format: str = None) -> str:
    """
    Generate a license key based on custom format or default format.
    
    Format rules:
    - Use * for random uppercase letters/numbers
    - Any other characters are kept as-is
    - Default format: XXXX-XXXX-XXXX-XXXX
    
    Examples:
    - "auth-****" -> "auth-A3B9"
    - "****" -> "K7M2"
    - "prefix-****-****" -> "prefix-X9Y2-P4Q8"
    """
    chars = string.ascii_uppercase + string.digits
    
    if custom_format:
        # Replace each * with a random character
        result = ""
        for char in custom_format:
            if char == '*':
                result += secrets.choice(chars)
            else:
                result += char
        return result
    else:
        # Default format: XXXX-XXXX-XXXX-XXXX
        parts = [''.join(secrets.choice(chars) for _ in range(4)) for _ in range(4)]
        return '-'.join(parts)

async def get_user_by_identifier(guild_id: int, identifier: str):
    """Get user data by either discord_id or license_key"""
    conn = get_db_connection(guild_id)
    if not conn:
        return None, None
    
    try:
        cur = conn.cursor()
        # Try as discord_id first (if it's numeric)
        if identifier.isdigit():
            cur.execute(
                f"SELECT discord_id, license_key, hwid, expiry_date, is_banned, last_ip, reset_count, created_at, is_paused, activated_at, duration_seconds "
                f"FROM {get_table_prefix(guild_id)}_users WHERE discord_id = %s",
                (int(identifier),)
            )
        else:
            # Try as license_key
            cur.execute(
                f"SELECT discord_id, license_key, hwid, expiry_date, is_banned, last_ip, reset_count, created_at, is_paused, activated_at, duration_seconds "
                f"FROM {get_table_prefix(guild_id)}_users WHERE license_key = %s",
                (identifier,)
            )
        
        result = cur.fetchone()
        return result, conn
    except Exception as e:
        logger.error(f"Error in get_user_by_identifier: {e}")
        release_db_connection(guild_id, conn)
        return None, None

async def safe_send_followup(interaction, content: str = None, embed: discord.Embed = None, file: discord.File = None):
    """Safely send followup message with error handling"""
    try:
        # Filter out None values to prevent the to_dict error
        kwargs = {}
        if content is not None:
            kwargs['content'] = content
        if embed is not None:
            kwargs['embed'] = embed
        if file is not None:
            kwargs['file'] = file
        
        await interaction.followup.send(**kwargs)
    except discord.errors.HTTPException as e:
        logger.error(f"Failed to send followup: {e}")
        if not interaction.response.is_done():
            try:
                await interaction.response.send_message("❌ Failed to send response.", ephemeral=True)
            except Exception as e:
                logger.error(f"Unexpected error in safe_send_followup: {e}")
    except discord.errors.NotFound:
        # Interaction expired - this is normal for old interactions
        pass
    except Exception as e:
        logger.error(f"Unexpected error in safe_send_followup: {e}")

# --- MODERN UI & LOGGING ENGINE ---

def create_modern_embed(title: str, color: int = 0xFFFFFF, guild_id: int = None):
    """Create a modern embed with consistent white styling and dynamic server info"""
    emb = discord.Embed(
        title=title,
        color=color,  # Always white
        timestamp=datetime.now()  # Use datetime object directly
    )
    
    # Get server info if guild_id provided
    if guild_id:
        guild = client.get_guild(guild_id)
        if guild:
            server_name = guild.name
            server_icon = guild.icon.url if guild.icon else "https://cdn.discordapp.com/attachments/1174385344375912448/1174385399505117225/SebwettSQL.png"
            footer_text = f"{server_name} | System"
            # Set thumbnail to server icon
            emb.set_thumbnail(url=server_icon)
        else:
            server_name = "Unknown Server"
            server_icon = "https://cdn.discordapp.com/attachments/1174385344375912448/1174385399505117225/SebwettSQL.png"
            footer_text = "Unknown Server | System"
            # Set thumbnail to default icon
            emb.set_thumbnail(url=server_icon)
    else:
        server_name = "Unknown Server"
        server_icon = "https://cdn.discordapp.com/attachments/1174385344375912448/1174385399505117225/SebwettSQL.png"
        footer_text = "Unknown Server | System"
        # Set thumbnail to default icon
        emb.set_thumbnail(url=server_icon)
    
    emb.set_footer(text=footer_text)
    return emb

async def dispatch_log(guild_id, embed):
    max_retries = 3
    for attempt in range(max_retries):
        try:
            m_conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
            cur = m_conn.cursor()
            
            # Check if this server is linked to another server
            cur.execute("SELECT linked_from_guild_id FROM server_configs WHERE guild_id = %s", (guild_id,))
            link_result = cur.fetchone()
            
            # If linked, use the original server's logging channel
            if link_result and link_result[0]:
                target_guild_id = link_result[0]
                logger.info(f"Server {guild_id} is linked to {target_guild_id}, using original server's logging channel")
            else:
                target_guild_id = guild_id
            
            # Get logging channel for the target guild
            cur.execute("SELECT channel_id FROM logging_config WHERE guild_id = %s", (target_guild_id,))
            res = cur.fetchone()
            m_conn.close()
            
            if res:
                logger.info(f"Found logging channel {res[0]} for guild {target_guild_id}")
                channel = client.get_channel(res[0])
                if channel: 
                    logger.info(f"Sending log to channel {channel.name} ({res[0]})")
                    # Don't override timestamp - embed already has it from create_modern_embed
                    await channel.send(embed=embed)
                    logger.info(f"Successfully sent log to guild {target_guild_id}")
                    break  # Success, exit retry loop
                else:
                    logger.warning(f"Could not find channel {res[0]} for guild {target_guild_id}")
            else:
                logger.warning(f"No logging channel configured for guild {target_guild_id}")
                break  # No point retrying if no config exists
        except Exception as e:
            logger.error(f"Logging Dispatch Error, attempt {attempt + 1}/{max_retries}: {e}")
            if attempt < max_retries - 1:
                await asyncio.sleep(0.1 * (attempt + 1))  # Brief delay before retry
            else:
                logger.error(f"Failed to dispatch log after {max_retries} attempts")

async def log_loader_event(guild_id, user_id, event_type, hwid=None, ip=None, additional_info=None):
    """Enhanced logging for loader events with professional formatting"""
    try:
        # Handle both Discord IDs and license keys
        is_license_key = False
        try:
            user_id_int = int(user_id)
            user = client.get_user(user_id_int)
            user_mention = f"<@{user_id}>"
            user_name = user.name if user else f"User {user_id}"
        except (ValueError, TypeError):
            # It's a license key, not a Discord ID
            is_license_key = True
            user = None
            user_mention = "N/A"
            user_name = f"License Key: {user_id}"
        
        # Log debug information
        logger.info(f"[LOADER EVENT] Guild: {guild_id} | User: {user_name} | Type: {event_type} | IP: {ip} | HWID: {hwid} | Info: {additional_info}")
        
        # Determine embed title based on event type (professional, no emojis)
        if event_type == "login_success":
            title = "Login Verification"
        elif event_type == "login_failed":
            title = "Login Verification"
        elif event_type == "loader_opened":
            title = "Application Started"
        else:
            title = "System Event"
        
        # Create professional embed matching /profile format
        emb = create_modern_embed(title, guild_id=guild_id)
        
        # Status field (like /profile)
        if event_type == "login_success":
            status = "SUCCESS"
        elif event_type == "login_failed":
            status = "FAILED"
        elif event_type == "loader_opened":
            status = "OPENED"
        else:
            status = "UNKNOWN"
        
        emb.add_field(name="Status", value=f"``{status}``", inline=False)
        
        # Show appropriate identifier based on type
        if is_license_key:
            emb.add_field(name="License Key", value=f"```{user_id}```", inline=False)
        else:
            emb.add_field(name="Login ID", value=f"```{user_id}```", inline=False)
            emb.add_field(name="Username", value=user_mention, inline=False)
        
        # Technical details in list format (not side by side)
        if hwid:
            emb.add_field(name="HWID", value=f"``{hwid}``", inline=False)
        if ip:
            emb.add_field(name="Last IP", value=f"``{ip}``", inline=False)
        
        # Additional information (reason for failure, etc.)
        if additional_info:
            emb.add_field(name="Details", value=f"``{additional_info}``", inline=False)
        
        # Add timestamp
        emb.timestamp = datetime.now()
        
        # Set thumbnail to user's profile picture (consistent with /profile style)
        if user and user.avatar:
            emb.set_thumbnail(url=user.avatar.url)
        elif user:
            emb.set_thumbnail(url=user.default_avatar.url)
        
        # Dispatch to logging channel
        await dispatch_log(guild_id, emb)
        logger.info(f"[LOADER LOG] Successfully dispatched log for {event_type} by user {user_id}")
        
    except Exception as e:
        logger.error(f"[LOADER ERROR] Failed to log loader event: {e}")

# --- AUTHENTICATION SYSTEM ---

async def is_superadmin(user_id: int) -> bool:
    """Check if user is the real superadmin (sebwett only)"""
    return user_id == SEBWETT_ID

async def is_admin(user_id: int):
    """Check if user is an admin (can do admin commands like /gen, /ban, etc.)"""
    if user_id == SEBWETT_ID: 
        return True
    try:
        conn = psycopg2.connect(MASTER_DB_URL)
        cur = conn.cursor()
        cur.execute("SELECT user_id FROM bot_admins WHERE user_id = %s", (user_id,))
        res = cur.fetchone()
        conn.close()
        return res is not None
    except: 
        return False

async def is_superadmin_user(user_id: int) -> bool:
    """Check if user is a superadmin (can do /link, /setup, /unlink, etc.)"""
    if user_id == SEBWETT_ID:
        return True
    try:
        conn = psycopg2.connect(MASTER_DB_URL)
        cur = conn.cursor()
        cur.execute("SELECT user_id FROM bot_superadmins WHERE user_id = %s", (user_id,))
        res = cur.fetchone()
        conn.close()
        return res is not None
    except Exception as e:
        logger.error(f"Error checking superadmin status: {e}")
        return False

def get_db(guild_id):
    """Get database connection with improved error handling and security"""
    max_retries = 3
    for attempt in range(max_retries):
        try:
            if not validate_user_id(guild_id):
                logger.warning(f"Invalid guild_id: {guild_id}")
                return None
                
            m_conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=15)
            m_cur = m_conn.cursor()
            
            # Use parameterized query to prevent SQL injection
            m_cur.execute("SELECT db_url FROM server_configs WHERE guild_id = %s", (guild_id,))
            res = m_cur.fetchone()
            m_conn.close()
            
            if not res: 
                logger.info(f"No database configuration found for guild {guild_id}")
                return None
                
            # Validate database URL format
            db_url = res[0]
            if not db_url.startswith('postgresql://') and not db_url.startswith('postgres://'):
                logger.error(f"Invalid database URL format for guild {guild_id}")
                return None
                
            conn = psycopg2.connect(db_url, connect_timeout=15)
            # Test the connection
            test_cur = conn.cursor()
            test_cur.execute("SELECT 1")
            test_cur.close()
            return conn
        except psycopg2.Error as e:
            logger.error(f"Database connection error for guild {guild_id}, attempt {attempt + 1}/{max_retries}: {e}")
            if attempt < max_retries - 1:
                time.sleep(0.1 * (attempt + 1))  # Brief delay before retry
            else:
                logger.error(f"Failed to connect to database for guild {guild_id} after {max_retries} attempts")
                return None
        except Exception as e:
            logger.error(f"Unexpected error in get_db for guild {guild_id}, attempt {attempt + 1}/{max_retries}: {e}")
            if attempt < max_retries - 1:
                time.sleep(0.1 * (attempt + 1))
            else:
                return None
    return None

# --- [1] ADMIN MANAGEMENT ---

@tree.command(name="admin_add", description="Grant admin access to a user")
async def admin_add(interaction: discord.Interaction, user: discord.User):
    await interaction.response.defer()
    # Allow both superadmins and the real superadmin to add admins
    if not await is_superadmin_user(interaction.user.id): 
        return await interaction.followup.send("Forbidden.")
    conn = psycopg2.connect(MASTER_DB_URL); cur = conn.cursor()
    cur.execute("INSERT INTO bot_admins (user_id, added_by) VALUES (%s, %s) ON CONFLICT DO NOTHING", (user.id, interaction.user.id))
    conn.commit(); conn.close()
    emb = create_modern_embed("Admin Added", guild_id=interaction.guild_id)
    emb.add_field(name="User", value=user.mention, inline=True)
    emb.add_field(name="Authorized By", value=interaction.user.mention, inline=True)
    await interaction.followup.send(embed=emb)
    await dispatch_log(interaction.guild_id, emb)

@tree.command(name="admin_remove", description="Revoke admin access")
async def admin_remove(interaction: discord.Interaction, user: discord.User):
    await interaction.response.defer()
    # Allow both superadmins and the real superadmin to remove admins
    if not await is_superadmin_user(interaction.user.id): 
        return await interaction.followup.send("Forbidden.")
    conn = psycopg2.connect(MASTER_DB_URL); cur = conn.cursor()
    cur.execute("DELETE FROM bot_admins WHERE user_id = %s", (user.id,))
    conn.commit(); conn.close()
    emb = create_modern_embed("Admin Revoked", guild_id=interaction.guild_id)
    emb.add_field(name="User", value=user.mention, inline=True)
    emb.add_field(name="Action By", value=interaction.user.mention, inline=True)
    await interaction.followup.send(embed=emb)
    await dispatch_log(interaction.guild_id, emb)

@tree.command(name="admin_list", description="List all authorized system admins")
async def admin_list(interaction: discord.Interaction):
    await interaction.response.defer()
    # Allow superadmins to view admin list
    if not await is_superadmin_user(interaction.user.id) and not await is_admin(interaction.user.id): 
        return await interaction.followup.send("Unauthorized.")
    conn = psycopg2.connect(MASTER_DB_URL); cur = conn.cursor()
    cur.execute("SELECT user_id FROM bot_admins")
    admins = cur.fetchall(); conn.close()
    
    # Build list with truncation if too long (max 1024 chars for embed field)
    admin_items = [f"• <@{a[0]}> ({a[0]})" for a in admins]
    list_str = "\n".join(admin_items) if admin_items else "No secondary admins."
    
    # Truncate if too long
    if len(list_str) > 1024:
        list_str = list_str[:1021] + "..."
    
    emb = create_modern_embed("System Administrators", guild_id=interaction.guild_id)
    emb.add_field(name="Personnel", value=list_str)
    await interaction.followup.send(embed=emb)

# --- SUPERADMIN MANAGEMENT ---

@tree.command(name="superadmin_add", description="Grant superadmin access to a user (sebwett only)")
async def superadmin_add(interaction: discord.Interaction, user: discord.User):
    await interaction.response.defer()
    if not await is_superadmin(interaction.user.id): 
        return await interaction.followup.send("Forbidden. Only sebwett can manage superadmins.")
    
    try:
        conn = psycopg2.connect(MASTER_DB_URL)
        cur = conn.cursor()
        
        # Create table if it doesn't exist
        cur.execute("""
            CREATE TABLE IF NOT EXISTS bot_superadmins (
                user_id BIGINT PRIMARY KEY,
                added_by BIGINT NOT NULL,
                added_at TIMESTAMP DEFAULT NOW()
            )
        """)
        
        # Add superadmin
        cur.execute(
            "INSERT INTO bot_superadmins (user_id, added_by) VALUES (%s, %s) ON CONFLICT DO NOTHING",
            (user.id, interaction.user.id)
        )
        conn.commit()
        conn.close()
        
        emb = create_modern_embed("Superadmin Added", guild_id=interaction.guild_id)
        emb.add_field(name="User", value=user.mention, inline=True)
        emb.add_field(name="Authorized By", value=interaction.user.mention, inline=True)
        emb.description = "This user can now use /link, /setup, /unlink, and manage admins."
        await interaction.followup.send(embed=emb)
        
        # Log to all configured servers
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
    except Exception as e:
        logger.error(f"Error adding superadmin: {e}")
        await interaction.followup.send("Failed to add superadmin.")

@tree.command(name="superadmin_remove", description="Revoke superadmin access (sebwett only)")
async def superadmin_remove(interaction: discord.Interaction, user: discord.User):
    await interaction.response.defer()
    if not await is_superadmin(interaction.user.id): 
        return await interaction.followup.send("Forbidden. Only sebwett can manage superadmins.")
    
    try:
        conn = psycopg2.connect(MASTER_DB_URL)
        cur = conn.cursor()
        cur.execute("DELETE FROM bot_superadmins WHERE user_id = %s", (user.id,))
        conn.commit()
        conn.close()
        
        emb = create_modern_embed("Superadmin Revoked", guild_id=interaction.guild_id)
        emb.add_field(name="User", value=user.mention, inline=True)
        emb.add_field(name="Action By", value=interaction.user.mention, inline=True)
        emb.description = "This user can no longer use superadmin commands."
        await interaction.followup.send(embed=emb)
        
        # Log to all configured servers
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
    except Exception as e:
        logger.error(f"Error removing superadmin: {e}")
        await interaction.followup.send("Failed to remove superadmin.")

@tree.command(name="superadmin_list", description="List all superadmins")
async def superadmin_list(interaction: discord.Interaction):
    await interaction.response.defer()
    if not await is_superadmin(interaction.user.id): 
        return await interaction.followup.send("Unauthorized.")
    
    try:
        conn = psycopg2.connect(MASTER_DB_URL)
        cur = conn.cursor()
        
        # Create table if it doesn't exist
        cur.execute("""
            CREATE TABLE IF NOT EXISTS bot_superadmins (
                user_id BIGINT PRIMARY KEY,
                added_by BIGINT NOT NULL,
                added_at TIMESTAMP DEFAULT NOW()
            )
        """)
        
        cur.execute("SELECT user_id FROM bot_superadmins")
        superadmins = cur.fetchall()
        conn.close()
        
        list_str = "\n".join([f"• <@{s[0]}> ({s[0]})" for s in superadmins]) or "No superadmins added."
        emb = create_modern_embed("System Superadmins", guild_id=interaction.guild_id)
        emb.add_field(name="Real Superadmin", value=f"• <@{SEBWETT_ID}> ({SEBWETT_ID})", inline=False)
        emb.add_field(name="Additional Superadmins", value=list_str, inline=False)
        emb.description = "Superadmins can use /link, /setup, /unlink, and manage admins."
        await interaction.followup.send(embed=emb)
    except Exception as e:
        logger.error(f"Error listing superadmins: {e}")
        await interaction.followup.send("Failed to list superadmins.")

@tree.command(name="setformat", description="Set custom license key format for this server")
@app_commands.describe(
    format="License key format (use * for random uppercase letters/numbers, e.g., 'auth-****' or '****-****')"
)
async def setformat(interaction: discord.Interaction, format: str):
    """Set custom license key format that persists across generations"""
    await interaction.response.defer()
    
    if not await is_admin(interaction.user.id):
        return await interaction.followup.send("Unauthorized.", ephemeral=True)
    
    # Validate format
    if not format or len(format) > 100:
        return await interaction.followup.send("Invalid format. Must be between 1-100 characters.", ephemeral=True)
    
    if '*' not in format:
        return await interaction.followup.send("Format must contain at least one * for random generation.", ephemeral=True)
    
    try:
        # Save format to database
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
        cur = conn.cursor()
        
        # Update or insert the format
        cur.execute(
            "UPDATE server_configs SET license_key_format = %s WHERE guild_id = %s",
            (format, interaction.guild_id)
        )
        
        conn.commit()
        conn.close()
        
        # Generate a preview
        preview = generate_license_key(format)
        
        emb = create_modern_embed("License Key Format Updated", guild_id=interaction.guild_id)
        emb.add_field(name="Format", value=f"``{format}``", inline=False)
        emb.add_field(name="Preview", value=f"``{preview}``", inline=False)
        emb.add_field(name="Updated By", value=interaction.user.mention, inline=True)
        emb.description = "This format will be used for all future license key generations.\n\n**Rules:**\n• Use `*` for random uppercase letters/numbers\n• Any other characters stay as-is"
        
        await interaction.followup.send(embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
    except Exception as e:
        logger.error(f"Error in setformat command: {e}")
        await interaction.followup.send("Failed to update license key format.")

@tree.command(name="getformat", description="View current license key format for this server")
async def getformat(interaction: discord.Interaction):
    """View the current license key format"""
    await interaction.response.defer()
    
    if not await is_admin(interaction.user.id):
        return await interaction.followup.send("Unauthorized.", ephemeral=True)
    
    try:
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
        cur = conn.cursor()
        
        cur.execute("SELECT license_key_format FROM server_configs WHERE guild_id = %s", (interaction.guild_id,))
        result = cur.fetchone()
        conn.close()
        
        if result and result[0]:
            format_str = result[0]
            preview = generate_license_key(format_str)
            
            emb = create_modern_embed("Current License Key Format", guild_id=interaction.guild_id)
            emb.add_field(name="Format", value=f"``{format_str}``", inline=False)
            emb.add_field(name="Preview", value=f"``{preview}``", inline=False)
        else:
            emb = create_modern_embed("Current License Key Format", guild_id=interaction.guild_id)
            emb.add_field(name="Format", value="``XXXX-XXXX-XXXX-XXXX`` (Default)", inline=False)
            emb.add_field(name="Preview", value=f"``{generate_license_key()}``", inline=False)
            emb.description = "Using default format. Use `/setformat` to customize."
        
        await interaction.followup.send(embed=emb)
        
    except Exception as e:
        logger.error(f"Error in getformat command: {e}")
        await interaction.followup.send("Failed to retrieve license key format.")

# --- [2] USER MANAGEMENT ---

@requires_config
@tree.command(name="gen", description="Generate or extend a user license")
@app_commands.describe(
    member="(Optional) The Discord user to link license to",
    days="Duration: 1h-23h for hours, 1-3650 for days (e.g., '5h' or '30')",
    role="(Optional) Role to assign to Discord user",
    quantity="(Optional) Number of license keys to generate (1-100, only if no member specified)"
)
async def gen(interaction: discord.Interaction, days: str, member: Optional[discord.Member] = None, role: Optional[discord.Role] = None, quantity: Optional[int] = 1):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    # Parse days parameter to support hours (1h-23h) or days (1-3650)
    duration_hours = None
    duration_days = None
    
    days_str = str(days).strip().lower()
    if days_str.endswith('h'):
        # Hour format (e.g., "5h")
        try:
            duration_hours = int(days_str[:-1])
            if duration_hours < 1 or duration_hours > 23:
                return await interaction.response.send_message("Invalid hours. Must be between 1h and 23h.", ephemeral=True)
        except ValueError:
            return await interaction.response.send_message("Invalid hour format. Use format like '5h' for 5 hours.", ephemeral=True)
    else:
        # Day format (e.g., "30")
        try:
            duration_days = int(days_str)
            if duration_days < 1 or duration_days > 3650:
                return await interaction.response.send_message("Invalid days. Must be between 1 and 3650.", ephemeral=True)
        except ValueError:
            return await interaction.response.send_message("Invalid format. Use '1h-23h' for hours or '1-3650' for days.", ephemeral=True)
    
    if member and not validate_user_id(member.id):
        return await interaction.response.send_message("Invalid user ID.", ephemeral=True)
    
    if not member and (quantity < 1 or quantity > 100):
        return await interaction.response.send_message("Quantity must be between 1 and 100.", ephemeral=True)
    
    # Defer only after initial validation
    await interaction.response.defer()
    
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    try:
        # Calculate duration in seconds
        if duration_hours:
            duration_seconds = duration_hours * 3600
            duration_display = f"{duration_hours}h"
            expiry = datetime.now() + timedelta(hours=duration_hours)
        else:
            duration_seconds = duration_days * 86400
            duration_display = f"{duration_days} days"
            expiry = datetime.now() + timedelta(days=duration_days)
        
        cur = conn.cursor()
        
        # Discord user mode
        if member:
            # For Discord users, set expiry immediately and mark as activated (backward compatibility)
            cur.execute(
                f"INSERT INTO {get_table_prefix(interaction.guild_id)}_users (discord_id, expiry_date, activated_at, duration_seconds) VALUES (%s, %s, %s, %s) "
                f"ON CONFLICT (discord_id) DO UPDATE SET expiry_date = GREATEST({get_table_prefix(interaction.guild_id)}_users.expiry_date, %s), duration_seconds = %s",
                (member.id, expiry, datetime.now(), duration_seconds, expiry, duration_seconds)
            )
            conn.commit()
            
            # Assign role if provided
            if role:
                try:
                    await member.add_roles(role)
                except Exception as e:
                    logger.error(f"Failed to assign role: {e}")
            
            emb = create_modern_embed("Profile Generation", guild_id=interaction.guild_id)
            emb.add_field(name="Admin", value=interaction.user.mention, inline=True)
            emb.add_field(name="User", value=member.mention, inline=True)
            emb.add_field(name="Login ID", value=f"```{member.id}```", inline=False)
            emb.add_field(name="Duration", value=f"``{duration_display}``", inline=True)
            emb.add_field(name="Expiration", value=f"`{expiry.strftime('%Y-%m-%d %H:%M')}`", inline=False)
            await safe_send_followup(interaction, embed=emb)
            
            # Non-blocking logging
            asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
        # License key mode (bulk generation)
        else:
            # Get custom format from database
            custom_format = None
            try:
                m_conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
                m_cur = m_conn.cursor()
                m_cur.execute("SELECT license_key_format FROM server_configs WHERE guild_id = %s", (interaction.guild_id,))
                format_result = m_cur.fetchone()
                m_conn.close()
                if format_result and format_result[0]:
                    custom_format = format_result[0]
            except Exception as e:
                logger.error(f"Failed to fetch custom format: {e}")
            
            generated_keys = []
            for _ in range(quantity):
                license_key = generate_license_key(custom_format)
                try:
                    # Store duration_seconds only, NOT expiry_date
                    # The expiry will be calculated on first activation
                    cur.execute(
                        f"INSERT INTO {get_table_prefix(interaction.guild_id)}_users (license_key, duration_seconds) VALUES (%s, %s)",
                        (license_key, duration_seconds)
                    )
                    generated_keys.append(license_key)
                except Exception as e:
                    logger.error(f"Failed to generate license key: {e}")
                    # Rollback the transaction on error
                    conn.rollback()
                    # Check if it's a missing column error
                    if "column" in str(e) and "does not exist" in str(e):
                        release_db_connection(interaction.guild_id, conn)
                        return await safe_send_followup(
                            interaction, 
                            "❌ Database needs migration. Please run `/migrate_db` first to enable license key support."
                        )
                    break
            
            if not generated_keys:
                return await safe_send_followup(interaction, "Failed to generate any license keys.")
            
            conn.commit()
            
            # Create embed with keys
            emb = create_modern_embed("License Keys Generated", guild_id=interaction.guild_id)
            emb.add_field(name="Admin", value=interaction.user.mention, inline=True)
            emb.add_field(name="Quantity", value=f"``{len(generated_keys)}``", inline=True)
            emb.add_field(name="Duration", value=f"``{duration_display}``", inline=True)
            emb.add_field(name="Status", value="``⏸️ Not Activated``", inline=False)
            emb.description = "⚠️ Duration will start counting down after first activation."
            
            # Add keys in a code block
            keys_text = "\n".join(generated_keys)
            emb.add_field(name="License Keys", value=f"```{keys_text}```", inline=False)
            
            await safe_send_followup(interaction, embed=emb)
            
            # Non-blocking logging with keys included
            log_emb = create_modern_embed("License Keys Generated", guild_id=interaction.guild_id)
            log_emb.add_field(name="Admin", value=interaction.user.mention, inline=True)
            log_emb.add_field(name="Quantity", value=f"``{len(generated_keys)}``", inline=True)
            log_emb.add_field(name="Duration", value=f"``{duration_display}``", inline=True)
            log_emb.add_field(name="Status", value="``⏸️ Not Activated``", inline=False)
            # Add keys to log embed as well
            log_emb.add_field(name="License Keys", value=f"```{keys_text}```", inline=False)
            asyncio.create_task(dispatch_log(interaction.guild_id, log_emb))
        
    except Exception as e:
        logger.error(f"Error in gen command: {e}")
        await safe_send_followup(interaction, "Failed to generate license.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="bulkgen", description="Bulk generate license keys for all durations (1d, 7d, 30d, lifetime)")
@app_commands.describe(
    quantity="Number of keys to generate per duration (1-100)"
)
async def bulkgen(interaction: discord.Interaction, quantity: int = 10):
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)

    if quantity < 1 or quantity > 100:
        return await interaction.response.send_message("Quantity must be between 1 and 100.", ephemeral=True)

    await interaction.response.defer()

    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    durations = [
        ("1 Day",    1 * 86400),
        ("7 Days",   7 * 86400),
        ("30 Days",  30 * 86400),
        ("Lifetime", 36500 * 86400),  # 100 years = lifetime
    ]

    # Get custom key format
    custom_format = None
    try:
        m_conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
        m_cur = m_conn.cursor()
        m_cur.execute("SELECT license_key_format FROM server_configs WHERE guild_id = %s", (interaction.guild_id,))
        fmt = m_cur.fetchone()
        m_conn.close()
        if fmt and fmt[0]:
            custom_format = fmt[0]
    except Exception as e:
        logger.error(f"Failed to fetch custom format: {e}")

    try:
        cur = conn.cursor()
        all_keys = {}

        for label, duration_seconds in durations:
            keys = []
            for _ in range(quantity):
                key = generate_license_key(custom_format)
                try:
                    cur.execute(
                        f"INSERT INTO {get_table_prefix(interaction.guild_id)}_users (license_key, duration_seconds) VALUES (%s, %s)",
                        (key, duration_seconds)
                    )
                    keys.append(key)
                except Exception as e:
                    logger.error(f"Failed to insert key: {e}")
                    conn.rollback()
                    break
            all_keys[label] = keys

        conn.commit()

        # Build text file content
        lines = [f"xim.gg License Keys — Generated {datetime.now().strftime('%Y-%m-%d %H:%M')}\n"]
        lines.append("=" * 50 + "\n")
        total = 0
        for label, keys in all_keys.items():
            lines.append(f"\n[ {label} ] — {len(keys)} keys\n")
            lines.append("-" * 30 + "\n")
            for k in keys:
                lines.append(f"{k}\n")
            total += len(keys)

        file_content = "".join(lines)
        file_bytes = io.BytesIO(file_content.encode("utf-8"))
        file = discord.File(file_bytes, filename=f"ximgg_keys_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt")

        emb = create_modern_embed("Bulk Keys Generated", guild_id=interaction.guild_id)
        emb.add_field(name="Admin", value=interaction.user.mention, inline=True)
        emb.add_field(name="Per Duration", value=f"``{quantity}``", inline=True)
        emb.add_field(name="Total Keys", value=f"``{total}``", inline=True)
        emb.add_field(name="Durations", value="``1 Day / 7 Days / 30 Days / Lifetime``", inline=False)
        emb.description = "⚠️ Duration starts counting after first activation."

        await safe_send_followup(interaction, embed=emb, file=file)
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))

    except Exception as e:
        logger.error(f"Error in bulkgen: {e}")
        await safe_send_followup(interaction, "Failed to bulk generate keys.")
    finally:
        release_db_connection(interaction.guild_id, conn)


@requires_config
@tree.command(name="ban", description="Blacklist a user or IP address")
@app_commands.describe(
    member="(Optional) Discord user to blacklist",
    license_key="(Optional) License key to blacklist",
    ip_address="(Optional) IP address to blacklist",
    reason="Reason for blacklisting"
)
async def ban(interaction: discord.Interaction, reason: str = "No reason provided", member: Optional[discord.Member] = None, license_key: Optional[str] = None, ip_address: Optional[str] = None):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    # Check that exactly one option is provided
    provided_options = sum([member is not None, license_key is not None, ip_address is not None])
    if provided_options == 0:
        return await interaction.response.send_message("You must provide either a Discord member, license key, or IP address.", ephemeral=True)
    
    if provided_options > 1:
        return await interaction.response.send_message("Please provide only one: Discord member, license key, OR IP address.", ephemeral=True)
    
    # Validate IP address format if provided
    if ip_address:
        import re
        ip_pattern = r'^(\d{1,3}\.){3}\d{1,3}$'
        if not re.match(ip_pattern, ip_address):
            return await interaction.response.send_message("Invalid IP address format. Use format: 192.168.1.1", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    # Handle IP ban separately
    if ip_address:
        try:
            conn = psycopg2.connect(MASTER_DB_URL)
            cur = conn.cursor()
            
            # Create IP ban table if it doesn't exist
            cur.execute("""
                CREATE TABLE IF NOT EXISTS ip_bans (
                    id SERIAL PRIMARY KEY,
                    guild_id BIGINT NOT NULL,
                    ip_address TEXT NOT NULL,
                    reason TEXT,
                    banned_by BIGINT NOT NULL,
                    banned_at TIMESTAMP DEFAULT NOW(),
                    UNIQUE(guild_id, ip_address)
                )
            """)
            
            # Add IP ban
            cur.execute(
                "INSERT INTO ip_bans (guild_id, ip_address, reason, banned_by) VALUES (%s, %s, %s, %s) ON CONFLICT (guild_id, ip_address) DO UPDATE SET reason = %s, banned_by = %s, banned_at = NOW()",
                (interaction.guild_id, ip_address, reason, interaction.user.id, reason, interaction.user.id)
            )
            conn.commit()
            conn.close()
            
            emb = create_modern_embed("IP Address Blacklisted", guild_id=interaction.guild_id, color=0xFFFFFF)
            emb.add_field(name="IP Address", value=f"```{ip_address}```", inline=False)
            emb.add_field(name="Reason", value=f"``{reason}``", inline=False)
            emb.add_field(name="Action By", value=interaction.user.mention, inline=True)
            emb.description = "All login attempts from this IP will be blocked."
            await safe_send_followup(interaction, embed=emb)
            
            # Non-blocking logging
            asyncio.create_task(dispatch_log(interaction.guild_id, emb))
            
        except Exception as e:
            logger.error(f"Error in IP ban: {e}")
            await safe_send_followup(interaction, "Failed to blacklist IP address.")
        return
    
    # Handle user/license ban
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    try:
        cur = conn.cursor()
        
        if member:
            cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_banned = TRUE WHERE discord_id = %s", (member.id,))
            identifier_display = f"```{member.id}```"
            user_display = member.mention
        else:
            cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_banned = TRUE WHERE license_key = %s", (license_key,))
            identifier_display = f"```{license_key}```"
            user_display = "Standalone License"
        
        conn.commit()
        
        emb = create_modern_embed("User Blacklisted", guild_id=interaction.guild_id, color=0xFFFFFF)
        emb.add_field(name="Target", value=user_display, inline=True)
        emb.add_field(name="Identifier", value=identifier_display, inline=False)
        emb.add_field(name="Reason", value=f"``{reason}``", inline=False)
        emb.add_field(name="Action By", value=interaction.user.mention, inline=True)
        await safe_send_followup(interaction, embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
    except Exception as e:
        logger.error(f"Error in ban command: {e}")
        await safe_send_followup(interaction, "Failed to blacklist user.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="unban", description="Remove user or IP address from blacklist")
@app_commands.describe(
    member="(Optional) Discord user to unblacklist",
    license_key="(Optional) License key to unblacklist",
    ip_address="(Optional) IP address to unblacklist"
)
async def unban(interaction: discord.Interaction, member: Optional[discord.Member] = None, license_key: Optional[str] = None, ip_address: Optional[str] = None):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    # Check that exactly one option is provided
    provided_options = sum([member is not None, license_key is not None, ip_address is not None])
    if provided_options == 0:
        return await interaction.response.send_message("You must provide either a Discord member, license key, or IP address.", ephemeral=True)
    
    if provided_options > 1:
        return await interaction.response.send_message("Please provide only one: Discord member, license key, OR IP address.", ephemeral=True)
    
    # Validate IP address format if provided
    if ip_address:
        import re
        ip_pattern = r'^(\d{1,3}\.){3}\d{1,3}$'
        if not re.match(ip_pattern, ip_address):
            return await interaction.response.send_message("Invalid IP address format. Use format: 192.168.1.1", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    # Handle IP unban separately
    if ip_address:
        try:
            conn = psycopg2.connect(MASTER_DB_URL)
            cur = conn.cursor()
            
            # Remove IP ban
            cur.execute(
                "DELETE FROM ip_bans WHERE guild_id = %s AND ip_address = %s",
                (interaction.guild_id, ip_address)
            )
            affected = cur.rowcount
            conn.commit()
            conn.close()
            
            if affected == 0:
                return await safe_send_followup(interaction, "IP address not found in blacklist.", ephemeral=True)
            
            emb = create_modern_embed("IP Address Reinstated", guild_id=interaction.guild_id, color=0xFFFFFF)
            emb.add_field(name="IP Address", value=f"```{ip_address}```", inline=False)
            emb.add_field(name="Action By", value=interaction.user.mention, inline=True)
            emb.description = "This IP address can now access the loader."
            await safe_send_followup(interaction, embed=emb)
            
            # Non-blocking logging
            asyncio.create_task(dispatch_log(interaction.guild_id, emb))
            
        except Exception as e:
            logger.error(f"Error in IP unban: {e}")
            await safe_send_followup(interaction, "Failed to unblacklist IP address.")
        return
    
    # Handle user/license unban
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    try:
        cur = conn.cursor()
        
        if member:
            cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_banned = FALSE WHERE discord_id = %s", (member.id,))
            identifier_display = f"```{member.id}```"
            user_display = member.mention
        else:
            cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_banned = FALSE WHERE license_key = %s", (license_key,))
            identifier_display = f"```{license_key}```"
            user_display = "Standalone License"
        
        conn.commit()
        
        emb = create_modern_embed("User Reinstated", guild_id=interaction.guild_id, color=0xFFFFFF)
        emb.add_field(name="User", value=user_display, inline=True)
        emb.add_field(name="Identifier", value=identifier_display, inline=False)
        emb.add_field(name="Action By", value=interaction.user.mention, inline=True)
        await safe_send_followup(interaction, embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
    except Exception as e:
        logger.error(f"Error in unban command: {e}")
        await safe_send_followup(interaction, "Failed to unblacklist user.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@tree.command(name="banlist", description="List all blacklisted users and IP addresses")
async def banlist(interaction: discord.Interaction):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    try:
        # Get banned users
        conn = get_db_connection(interaction.guild_id)
        banned_users = []
        if conn:
            cur = conn.cursor()
            cur.execute(
                f"SELECT discord_id, license_key FROM {get_table_prefix(interaction.guild_id)}_users WHERE is_banned = TRUE ORDER BY discord_id"
            )
            banned_users = cur.fetchall()
            release_db_connection(interaction.guild_id, conn)
        
        # Get banned IPs
        master_conn = psycopg2.connect(MASTER_DB_URL)
        master_cur = master_conn.cursor()
        
        # Create table if it doesn't exist
        master_cur.execute("""
            CREATE TABLE IF NOT EXISTS ip_bans (
                id SERIAL PRIMARY KEY,
                guild_id BIGINT NOT NULL,
                ip_address TEXT NOT NULL,
                reason TEXT,
                banned_by BIGINT NOT NULL,
                banned_at TIMESTAMP DEFAULT NOW(),
                UNIQUE(guild_id, ip_address)
            )
        """)
        
        master_cur.execute(
            "SELECT ip_address, reason FROM ip_bans WHERE guild_id = %s ORDER BY banned_at DESC",
            (interaction.guild_id,)
        )
        banned_ips = master_cur.fetchall()
        master_conn.close()
        
        # Create embed
        emb = create_modern_embed("Blacklist Overview", guild_id=interaction.guild_id)
        
        # Add banned users section
        if banned_users:
            user_list = []
            for discord_id, lic_key in banned_users:
                if discord_id:
                    user_list.append(f"• <@{discord_id}> (`{discord_id}`)")
                elif lic_key:
                    user_list.append(f"• License: `{lic_key}`")
            emb.add_field(name=f"Banned Users ({len(banned_users)})", value="\n".join(user_list[:10]) + ("\n..." if len(user_list) > 10 else ""), inline=False)
        else:
            emb.add_field(name="Banned Users (0)", value="No users banned", inline=False)
        
        # Add banned IPs section
        if banned_ips:
            ip_list = []
            for ip, reason in banned_ips:
                ip_list.append(f"• `{ip}` - {reason}")
            emb.add_field(name=f"Banned IPs ({len(banned_ips)})", value="\n".join(ip_list[:10]) + ("\n..." if len(ip_list) > 10 else ""), inline=False)
        else:
            emb.add_field(name="Banned IPs (0)", value="No IPs banned", inline=False)
        
        emb.description = f"Total Bans: {len(banned_users) + len(banned_ips)}"
        await safe_send_followup(interaction, embed=emb)
        
    except Exception as e:
        logger.error(f"Error in banlist command: {e}")
        await safe_send_followup(interaction, "Failed to list bans.")

@requires_config
@tree.command(name="delete", description="Delete user subscription completely")
@app_commands.describe(
    member="(Optional) Discord user to delete",
    license_key="(Optional) License key to delete"
)
async def delete(interaction: discord.Interaction, member: Optional[discord.Member] = None, license_key: Optional[str] = None):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    if not member and not license_key:
        return await interaction.response.send_message("You must provide either a Discord member or a license key.", ephemeral=True)
    
    if member and license_key:
        return await interaction.response.send_message("Please provide only one: Discord member OR license key, not both.", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    try:
        cur = conn.cursor()
        
        if member:
            cur.execute(f"DELETE FROM {get_table_prefix(interaction.guild_id)}_users WHERE discord_id = %s", (member.id,))
            identifier_display = f"```{member.id}```"
            user_display = member.mention
        else:
            cur.execute(f"DELETE FROM {get_table_prefix(interaction.guild_id)}_users WHERE license_key = %s", (license_key,))
            identifier_display = f"```{license_key}```"
            user_display = "Standalone License"
        
        conn.commit()
        
        emb = create_modern_embed("User Deleted", guild_id=interaction.guild_id, color=0xFFFFFF)
        emb.add_field(name="Target", value=user_display, inline=True)
        emb.add_field(name="Identifier", value=identifier_display, inline=False)
        emb.add_field(name="Status", value="``PERMANENTLY DELETED``", inline=False)
        emb.add_field(name="Action By", value=interaction.user.mention, inline=True)
        emb.description = "User has been completely removed from the database."
        await safe_send_followup(interaction, embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
    except Exception as e:
        logger.error(f"Error in delete command: {e}")
        await safe_send_followup(interaction, "Failed to delete user.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="edit", description="Edit user subscription days")
@app_commands.describe(member="The user to edit")
async def edit(interaction: discord.Interaction, member: discord.Member):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    try:
        cur = conn.cursor()
        cur.execute(f"SELECT expiry_date FROM {get_table_prefix(interaction.guild_id)}_users WHERE discord_id = %s", (member.id,))
        res = cur.fetchone()
        
        if not res:
            return await safe_send_followup(interaction, "User not found in database.")
        
        expiry = res[0]
        if expiry:
            # Calculate days remaining
            days_remaining = (expiry - datetime.now()).days
            if days_remaining < 0:
                days_remaining = 0
        else:
            days_remaining = 0
        
        # Create modal for editing
        class EditModal(discord.ui.Modal, title="Edit Subscription Days"):
            def __init__(self, member, current_days):
                super().__init__()
                self.member = member
                self.current_days = current_days
                
            days_input = discord.ui.TextInput(
                label="Days Remaining",
                placeholder=f"Current: {days_remaining} days",
                default=str(days_remaining),
                required=True,
                style=discord.TextStyle.short,
                max_length=10
            )
            
            async def on_submit(self, interaction: discord.Interaction):
                try:
                    new_days = int(self.days_input.value)
                    if new_days < 0:
                        return await interaction.response.send_message("Days cannot be negative.", ephemeral=True)
                    
                    # Update expiry date
                    new_expiry = datetime.now() + timedelta(days=new_days)
                    
                    conn = get_db_connection(interaction.guild_id)
                    if conn:
                        cur = conn.cursor()
                        cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET expiry_date = %s WHERE discord_id = %s", 
                                   (new_expiry, self.member.id))
                        conn.commit()
                        release_db_connection(interaction.guild_id, conn)
                        
                        emb = create_modern_embed("Subscription Updated", guild_id=interaction.guild_id, color=0xFFFFFF)
                        emb.add_field(name="User", value=self.member.mention, inline=True)
                        emb.add_field(name="Login ID", value=f"```{self.member.id}```", inline=False)
                        emb.add_field(name="Previous Days", value=f"``{self.current_days}``", inline=True)
                        emb.add_field(name="New Days", value=f"``{new_days}``", inline=True)
                        emb.add_field(name="New Expiry", value=f"``{new_expiry.strftime('%Y-%m-%d %H:%M')}``", inline=True)
                        emb.add_field(name="Edited By", value=interaction.user.mention, inline=True)
                        await interaction.response.send_message(embed=emb)
                        
                        # Log the change
                        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
                    else:
                        await interaction.response.send_message("Database error occurred.", ephemeral=True)
                        
                except ValueError:
                    await interaction.response.send_message("Invalid number format.", ephemeral=True)
                except Exception as e:
                    logger.error(f"Error in edit modal: {e}")
                    await interaction.response.send_message("Failed to update subscription.", ephemeral=True)
        
        await interaction.response.send_modal(EditModal(member, days_remaining))
        
    except Exception as e:
        logger.error(f"Error in edit command: {e}")
        await safe_send_followup(interaction, "Failed to fetch user data.")
    finally:
        if conn:
            release_db_connection(interaction.guild_id, conn)

@requires_config
@requires_config
@tree.command(name="reset", description="Clear user HWID lock")
@app_commands.describe(
    user="(Optional) Discord user to reset HWID for",
    license_key="(Optional) License key to reset HWID for"
)
async def reset(interaction: discord.Interaction, user: Optional[discord.User] = None, license_key: Optional[str] = None):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    if not user and not license_key:
        return await interaction.response.send_message("You must provide either a Discord user or a license key.", ephemeral=True)
    
    if user and license_key:
        return await interaction.response.send_message("Please provide only one: Discord user OR license key, not both.", ephemeral=True)
    
    await interaction.response.defer()
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    try:
        cur = conn.cursor()
        
        # Query and update by Discord ID or license key
        if user:
            cur.execute(f"SELECT hwid, license_key FROM {get_table_prefix(interaction.guild_id)}_users WHERE discord_id = %s", (user.id,))
            res = cur.fetchone()
            if not res:
                return await safe_send_followup(interaction, "User not found in database.")
            
            cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET hwid = NULL, reset_count = reset_count + 1 WHERE discord_id = %s", (user.id,))
            identifier_display = f"```{user.id}```"
            user_display = user.mention
        else:
            cur.execute(f"SELECT hwid, discord_id FROM {get_table_prefix(interaction.guild_id)}_users WHERE license_key = %s", (license_key,))
            res = cur.fetchone()
            if not res:
                return await safe_send_followup(interaction, "License key not found in database.")
            
            cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET hwid = NULL, reset_count = reset_count + 1 WHERE license_key = %s", (license_key,))
            identifier_display = f"```{license_key}```"
            user_display = "Standalone License"
        
        conn.commit()
        
        emb = create_modern_embed("HWID Reset", guild_id=interaction.guild_id)
        emb.add_field(name="User", value=user_display, inline=True)
        emb.add_field(name="Identifier", value=identifier_display, inline=False)
        emb.description = f"The hardware lock has been cleared.\nThey can now login from a new device."
        await safe_send_followup(interaction, embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
    except Exception as e:
        logger.error(f"Error in reset command: {e}")
        await safe_send_followup(interaction, "Failed to reset HWID.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@tree.command(name="reset_all", description="Clear ALL hardware locks for all users in database")
async def reset_all(interaction: discord.Interaction):
    # Quick validation first - only superadmins can use this
    if not await is_superadmin_user(interaction.user.id):
        return await interaction.response.send_message("Unauthorized. Only superadmins can reset all HWIDs.", ephemeral=True)
    
    # Defer immediately
    await interaction.response.defer()
    
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    try:
        cur = conn.cursor()
        
        # Get count of users with HWIDs before reset
        cur.execute(f"SELECT COUNT(*) FROM {get_table_prefix(interaction.guild_id)}_users WHERE hwid IS NOT NULL")
        count_before = cur.fetchone()[0]
        
        if count_before == 0:
            return await safe_send_followup(interaction, "No hardware locks found to reset.", ephemeral=True)
        
        # Reset all HWIDs and increment reset count
        cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET hwid = NULL, reset_count = reset_count + 1 WHERE hwid IS NOT NULL")
        affected = cur.rowcount
        conn.commit()
        
        emb = create_modern_embed("Global HWID Reset", guild_id=interaction.guild_id, color=0xFFFFFF)
        emb.add_field(name="Status", value="``ALL HARDWARE LOCKS CLEARED``", inline=False)
        emb.add_field(name="Users Affected", value=f"``{affected}``", inline=True)
        emb.add_field(name="Action By", value=interaction.user.mention, inline=True)
        emb.description = "All users can now login from new devices.\nHardware locks will be re-established on next login."
        await safe_send_followup(interaction, embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
        logger.info(f"[RESET ALL] {interaction.user} reset all HWIDs for guild {interaction.guild_id}, affected {affected} users")
        
    except Exception as e:
        logger.error(f"Error in reset_all command: {e}")
        await safe_send_followup(interaction, "Failed to reset all HWIDs.")
    finally:
        release_db_connection(interaction.guild_id, conn)

# --- BULK MANAGEMENT COMMANDS ---

@requires_config
@tree.command(name="delete_all_keys", description="Delete ALL unactivated license keys")
async def delete_all_keys(interaction: discord.Interaction):
    """Delete every license key that hasn't been activated yet"""
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    await interaction.response.defer()
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))
    
    try:
        cur = conn.cursor()
        # Count keys to delete
        cur.execute(
            f"SELECT COUNT(*) FROM {get_table_prefix(interaction.guild_id)}_users WHERE license_key IS NOT NULL AND activated_at IS NULL"
        )
        count = cur.fetchone()[0]
        
        if count == 0:
            return await safe_send_followup(interaction, "No unactivated license keys to delete.")
        
        # Delete unactivated license keys
        cur.execute(
            f"DELETE FROM {get_table_prefix(interaction.guild_id)}_users WHERE license_key IS NOT NULL AND activated_at IS NULL"
        )
        conn.commit()
        
        emb = create_modern_embed("All Unactivated Keys Deleted", guild_id=interaction.guild_id)
        emb.add_field(name="Admin", value=interaction.user.mention, inline=True)
        emb.add_field(name="Keys Deleted", value=f"``{count}``", inline=True)
        emb.description = "All unactivated license keys have been permanently removed."
        await safe_send_followup(interaction, embed=emb)
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        logger.info(f"[DELETE ALL KEYS] {interaction.user} deleted {count} keys in guild {interaction.guild_id}")
    except Exception as e:
        logger.error(f"Error in delete_all_keys: {e}")
        await safe_send_followup(interaction, "Failed to delete keys.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="delete_expired", description="Delete all expired licenses")
async def delete_expired(interaction: discord.Interaction):
    """Delete all users with expired licenses"""
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    await interaction.response.defer()
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))
    
    try:
        cur = conn.cursor()
        cur.execute(
            f"SELECT COUNT(*) FROM {get_table_prefix(interaction.guild_id)}_users WHERE expiry_date IS NOT NULL AND expiry_date < NOW()"
        )
        count = cur.fetchone()[0]
        
        if count == 0:
            return await safe_send_followup(interaction, "No expired licenses to delete.")
        
        cur.execute(
            f"DELETE FROM {get_table_prefix(interaction.guild_id)}_users WHERE expiry_date IS NOT NULL AND expiry_date < NOW()"
        )
        conn.commit()
        
        emb = create_modern_embed("Expired Licenses Deleted", guild_id=interaction.guild_id)
        emb.add_field(name="Admin", value=interaction.user.mention, inline=True)
        emb.add_field(name="Licenses Deleted", value=f"``{count}``", inline=True)
        emb.description = "All expired licenses have been permanently removed."
        await safe_send_followup(interaction, embed=emb)
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        logger.info(f"[DELETE EXPIRED] {interaction.user} deleted {count} expired licenses in guild {interaction.guild_id}")
    except Exception as e:
        logger.error(f"Error in delete_expired: {e}")
        await safe_send_followup(interaction, "Failed to delete expired licenses.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="delete_banned", description="Delete all banned users")
async def delete_banned(interaction: discord.Interaction):
    """Delete all blacklisted users from the database"""
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    await interaction.response.defer()
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))
    
    try:
        cur = conn.cursor()
        cur.execute(
            f"SELECT COUNT(*) FROM {get_table_prefix(interaction.guild_id)}_users WHERE is_banned = TRUE"
        )
        count = cur.fetchone()[0]
        
        if count == 0:
            return await safe_send_followup(interaction, "No banned users to delete.")
        
        cur.execute(
            f"DELETE FROM {get_table_prefix(interaction.guild_id)}_users WHERE is_banned = TRUE"
        )
        conn.commit()
        
        emb = create_modern_embed("Banned Users Purged", guild_id=interaction.guild_id)
        emb.add_field(name="Admin", value=interaction.user.mention, inline=True)
        emb.add_field(name="Users Deleted", value=f"``{count}``", inline=True)
        emb.description = "All banned users have been permanently removed."
        await safe_send_followup(interaction, embed=emb)
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        logger.info(f"[DELETE BANNED] {interaction.user} deleted {count} banned users in guild {interaction.guild_id}")
    except Exception as e:
        logger.error(f"Error in delete_banned: {e}")
        await safe_send_followup(interaction, "Failed to delete banned users.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="wipe_database", description="⚠️ DANGER: Delete ALL users from database (Superadmin only)")
async def wipe_database(interaction: discord.Interaction):
    """Wipe the entire user database for this server"""
    if not await is_superadmin_user(interaction.user.id):
        return await interaction.response.send_message("Unauthorized. Superadmin only.", ephemeral=True)
    
    await interaction.response.defer()
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))
    
    try:
        cur = conn.cursor()
        cur.execute(f"SELECT COUNT(*) FROM {get_table_prefix(interaction.guild_id)}_users")
        count = cur.fetchone()[0]
        
        if count == 0:
            return await safe_send_followup(interaction, "Database is already empty.")
        
        cur.execute(f"DELETE FROM {get_table_prefix(interaction.guild_id)}_users")
        conn.commit()
        
        emb = create_modern_embed("⚠️ Database Wiped", guild_id=interaction.guild_id)
        emb.add_field(name="Admin", value=interaction.user.mention, inline=True)
        emb.add_field(name="Records Deleted", value=f"``{count}``", inline=True)
        emb.description = "**ALL users have been permanently removed from this server's database.**"
        await safe_send_followup(interaction, embed=emb)
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        logger.warning(f"[WIPE DATABASE] {interaction.user} wiped {count} users in guild {interaction.guild_id}")
    except Exception as e:
        logger.error(f"Error in wipe_database: {e}")
        await safe_send_followup(interaction, "Failed to wipe database.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="unban_all", description="Remove ban status from all users")
async def unban_all(interaction: discord.Interaction):
    """Unban all blacklisted users"""
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    await interaction.response.defer()
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))
    
    try:
        cur = conn.cursor()
        cur.execute(
            f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_banned = FALSE WHERE is_banned = TRUE"
        )
        affected = cur.rowcount
        conn.commit()
        
        if affected == 0:
            return await safe_send_followup(interaction, "No banned users found.")
        
        emb = create_modern_embed("Mass Unban Complete", guild_id=interaction.guild_id)
        emb.add_field(name="Admin", value=interaction.user.mention, inline=True)
        emb.add_field(name="Users Unbanned", value=f"``{affected}``", inline=True)
        await safe_send_followup(interaction, embed=emb)
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
    except Exception as e:
        logger.error(f"Error in unban_all: {e}")
        await safe_send_followup(interaction, "Failed to unban all users.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="export_keys", description="Export all license keys as a text file")
async def export_keys(interaction: discord.Interaction):
    """Export all license keys to a downloadable file"""
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    await interaction.response.defer()
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))
    
    try:
        cur = conn.cursor()
        cur.execute(
            f"SELECT license_key, duration_seconds, activated_at, expiry_date, is_banned, is_paused "
            f"FROM {get_table_prefix(interaction.guild_id)}_users WHERE license_key IS NOT NULL ORDER BY created_at DESC"
        )
        rows = cur.fetchall()
        
        if not rows:
            return await safe_send_followup(interaction, "No license keys found.")
        
        lines = [f"xim.gg License Key Export — {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"]
        lines.append("=" * 70)
        lines.append(f"{'KEY':<25} {'DURATION':<12} {'STATUS':<15} {'EXPIRY':<20}")
        lines.append("-" * 70)
        
        for key, duration, activated, expiry, banned, paused in rows:
            if banned:
                status = "BANNED"
            elif paused:
                status = "PAUSED"
            elif activated is None:
                status = "INACTIVE"
            elif expiry and expiry < datetime.now():
                status = "EXPIRED"
            else:
                status = "ACTIVE"
            
            if duration:
                if duration >= 36500 * 86400:
                    dur_str = "LIFETIME"
                elif duration >= 86400:
                    dur_str = f"{duration // 86400}d"
                else:
                    dur_str = f"{duration // 3600}h"
            else:
                dur_str = "N/A"
            
            expiry_str = expiry.strftime('%Y-%m-%d %H:%M') if expiry else "Not Set"
            lines.append(f"{key:<25} {dur_str:<12} {status:<15} {expiry_str:<20}")
        
        lines.append("-" * 70)
        lines.append(f"Total: {len(rows)} keys")
        
        file_bytes = io.BytesIO("\n".join(lines).encode("utf-8"))
        file = discord.File(file_bytes, filename=f"keys_export_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt")
        
        emb = create_modern_embed("Keys Exported", guild_id=interaction.guild_id)
        emb.add_field(name="Total Keys", value=f"``{len(rows)}``", inline=True)
        emb.add_field(name="Admin", value=interaction.user.mention, inline=True)
        
        await safe_send_followup(interaction, embed=emb, file=file)
    except Exception as e:
        logger.error(f"Error in export_keys: {e}")
        await safe_send_followup(interaction, "Failed to export keys.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="keys", description="View all license keys with their status")
async def keys(interaction: discord.Interaction):
    """Display all license keys with their current status"""
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    await interaction.response.defer()
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))
    
    try:
        cur = conn.cursor()
        cur.execute(
            f"SELECT license_key, duration_seconds, activated_at, expiry_date, is_banned, is_paused, hwid "
            f"FROM {get_table_prefix(interaction.guild_id)}_users WHERE license_key IS NOT NULL ORDER BY created_at DESC"
        )
        rows = cur.fetchall()
        
        if not rows:
            return await safe_send_followup(interaction, "No license keys found.")
        
        # Count statistics
        total = len(rows)
        active_count = 0
        inactive_count = 0
        expired_count = 0
        banned_count = 0
        paused_count = 0
        
        for _, _, activated, expiry, banned, paused, _ in rows:
            if banned:
                banned_count += 1
            elif paused:
                paused_count += 1
            elif activated is None:
                inactive_count += 1
            elif expiry and expiry < datetime.now():
                expired_count += 1
            else:
                active_count += 1
        
        emb = create_modern_embed("License Keys Overview", guild_id=interaction.guild_id)
        emb.add_field(name="Total Keys", value=f"``{total}``", inline=True)
        emb.add_field(name="Active", value=f"``{active_count}``", inline=True)
        emb.add_field(name="Inactive", value=f"``{inactive_count}``", inline=True)
        emb.add_field(name="Expired", value=f"``{expired_count}``", inline=True)
        emb.add_field(name="Banned", value=f"``{banned_count}``", inline=True)
        emb.add_field(name="Paused", value=f"``{paused_count}``", inline=True)
        
        # Show keys list (limit to 20 in embed, full list in file if more)
        display_keys = rows[:20]
        keys_text = ""
        for key, duration, activated, expiry, banned, paused, hwid in display_keys:
            if banned:
                status_icon = "🚫"
            elif paused:
                status_icon = "⏸️"
            elif activated is None:
                status_icon = "⏳"
            elif expiry and expiry < datetime.now():
                status_icon = "❌"
            else:
                status_icon = "✅"
            
            hwid_icon = "🔒" if hwid else "🔓"
            keys_text += f"{status_icon} {hwid_icon} `{key}`\n"
        
        if keys_text:
            emb.add_field(name=f"Recent Keys (showing {len(display_keys)}/{total})", value=keys_text[:1024], inline=False)
        
        emb.add_field(
            name="Legend",
            value="✅ Active  •  ⏳ Inactive  •  ❌ Expired  •  🚫 Banned  •  ⏸️ Paused\n🔒 HWID Locked  •  🔓 No HWID",
            inline=False
        )
        
        # If there are more keys, attach as file
        if total > 20:
            lines = [f"All License Keys — {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"]
            lines.append("=" * 70)
            for key, duration, activated, expiry, banned, paused, hwid in rows:
                if banned:
                    status = "BANNED"
                elif paused:
                    status = "PAUSED"
                elif activated is None:
                    status = "INACTIVE"
                elif expiry and expiry < datetime.now():
                    status = "EXPIRED"
                else:
                    status = "ACTIVE"
                expiry_str = expiry.strftime('%Y-%m-%d %H:%M') if expiry else "Not Set"
                lines.append(f"{key:<25} {status:<10} {expiry_str}")
            
            file_bytes = io.BytesIO("\n".join(lines).encode("utf-8"))
            file = discord.File(file_bytes, filename=f"all_keys_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt")
            emb.description = f"📎 Full list of {total} keys attached as file."
            await safe_send_followup(interaction, embed=emb, file=file)
        else:
            await safe_send_followup(interaction, embed=emb)
    except Exception as e:
        logger.error(f"Error in keys command: {e}")
        await safe_send_followup(interaction, "Failed to fetch keys.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@tree.command(name="loader_opened", description="Log when a user opens the loader")
@app_commands.describe(
    user="The user who opened the loader",
    additional_info="Additional information about the event"
)
async def loader_opened(interaction: discord.Interaction, user: discord.User, additional_info: str = "No additional info"):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    try:
        # Log the loader opened event
        await log_loader_event(interaction.guild_id, user.id, "loader_opened", additional_info=additional_info)
        
        emb = create_modern_embed("🚪 Loader Event Logged", guild_id=interaction.guild_id, color=0x00AAFF)
        emb.description = f"Successfully logged loader opening event for {user.mention}"
        emb.add_field(name="User ID", value=f"```{user.id}```", inline=True)
        emb.add_field(name="Event Type", value="```Loader Opened```", inline=True)
        emb.add_field(name="Additional Info", value=f"```{additional_info}```", inline=False)
        
        await safe_send_followup(interaction, embed=emb)
        
    except Exception as e:
        logger.error(f"Error in loader_opened command: {e}")
        await safe_send_followup(interaction, "Failed to log loader event.")

@tree.command(name="profile", description="Query full diagnostic profile")
@app_commands.describe(
    member="(Optional) Discord user to query",
    license_key="(Optional) License key to query"
)
async def profile(interaction: discord.Interaction, member: Optional[discord.Member] = None, license_key: Optional[str] = None):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    if not member and not license_key:
        return await interaction.response.send_message("You must provide either a Discord member or a license key.", ephemeral=True)
    
    if member and license_key:
        return await interaction.response.send_message("Please provide only one: Discord member OR license key, not both.", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, "Database not linked.")
    
    try:
        cur = conn.cursor()
        
        # Query by Discord ID or license key
        if member:
            cur.execute(
                f"SELECT discord_id, license_key, hwid, expiry_date, is_banned, last_ip, reset_count, created_at, is_paused, activated_at, duration_seconds "
                f"FROM {get_table_prefix(interaction.guild_id)}_users WHERE discord_id = %s",
                (member.id,)
            )
            identifier_display = f"```{member.id}```"
            user_display = member.mention
        else:
            cur.execute(
                f"SELECT discord_id, license_key, hwid, expiry_date, is_banned, last_ip, reset_count, created_at, is_paused, activated_at, duration_seconds "
                f"FROM {get_table_prefix(interaction.guild_id)}_users WHERE license_key = %s",
                (license_key,)
            )
            identifier_display = f"```{license_key}```"
            user_display = "Standalone License"
        
        res = cur.fetchone()
        
        if not res:
            return await safe_send_followup(interaction, "Record not found.")
        
        discord_id, lic_key, hwid, expiry_date, is_banned, last_ip, reset_count, created_at, is_paused, activated_at, duration_seconds = res
        
        # Determine status with activation info
        if is_banned:
            status = "BLACKLISTED"
        elif is_paused:
            status = "PAUSED"
        elif activated_at is None and duration_seconds is not None:
            status = "NOT ACTIVATED"
        else:
            status = "ACTIVE"
        
        emb = create_modern_embed("Profile Generation", guild_id=interaction.guild_id)
        emb.add_field(name="Status", value=f"``{status}``", inline=False)
        
        # Show both Discord ID and License Key if available
        if discord_id:
            emb.add_field(name="Discord User", value=user_display if member else f"<@{discord_id}>", inline=False)
            emb.add_field(name="Login ID (Discord)", value=f"```{discord_id}```", inline=False)
        if lic_key:
            emb.add_field(name="License Key", value=f"```{lic_key}```", inline=False)
        
        emb.add_field(name="HWID", value=f"``{hwid or 'UNLINKED'}``", inline=False)
        emb.add_field(name="Resets", value=f"``{reset_count}``", inline=True)
        
        # Show activation status and expiry
        if activated_at:
            emb.add_field(name="Activated", value=f"``{activated_at.strftime('%Y-%m-%d %H:%M')}``", inline=True)
            if expiry_date:
                emb.add_field(name="Expiry", value=f"``{expiry_date.strftime('%Y-%m-%d %H:%M')}``", inline=True)
        elif duration_seconds:
            # Not activated yet, show duration
            days_duration = duration_seconds // 86400
            hours_duration = (duration_seconds % 86400) // 3600
            if hours_duration > 0:
                duration_str = f"{days_duration}d {hours_duration}h"
            else:
                duration_str = f"{days_duration} days"
            emb.add_field(name="Duration", value=f"``{duration_str}``", inline=True)
            emb.add_field(name="Expiry", value="``⏸️ Starts on first login``", inline=True)
        elif expiry_date:
            emb.add_field(name="Expiry", value=f"``{expiry_date.strftime('%Y-%m-%d %H:%M')}``", inline=True)
        
        emb.add_field(name="Last IP", value=f"``{last_ip or '0.0.0.0'}``", inline=True)
        await safe_send_followup(interaction, embed=emb)
        
    except Exception as e:
        logger.error(f"Error in profile command: {e}")
        await safe_send_followup(interaction, "Failed to fetch profile.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="comp", description="Add time to specific user")
@app_commands.describe(
    days="Number of days to add (1-3650)",
    member="(Optional) Discord user to compensate",
    license_key="(Optional) License key to compensate"
)
async def comp(interaction: discord.Interaction, days: int, member: Optional[discord.Member] = None, license_key: Optional[str] = None):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    if not validate_days(days):
        return await interaction.response.send_message("Invalid days parameter. Must be between 1 and 3650.", ephemeral=True)
    
    if not member and not license_key:
        return await interaction.response.send_message("You must provide either a Discord member or a license key.", ephemeral=True)
    
    if member and license_key:
        return await interaction.response.send_message("Please provide only one: Discord member OR license key, not both.", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    try:
        cur = conn.cursor()
        
        if member:
            cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET expiry_date = expiry_date + INTERVAL '%s days' WHERE discord_id = %s AND is_banned = FALSE", (days, member.id))
            identifier_display = f"```{member.id}```"
            user_display = member.mention
        else:
            cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET expiry_date = expiry_date + INTERVAL '%s days' WHERE license_key = %s AND is_banned = FALSE", (days, license_key))
            identifier_display = f"```{license_key}```"
            user_display = "Standalone License"
        
        affected = cur.rowcount
        conn.commit()
        
        if affected == 0:
            return await safe_send_followup(interaction, "User not found or is banned.")
        
        emb = create_modern_embed("User Compensation", guild_id=interaction.guild_id)
        emb.add_field(name="User", value=user_display, inline=True)
        emb.add_field(name="Identifier", value=identifier_display, inline=False)
        emb.add_field(name="Time Added", value=f"`{days} Days`", inline=True)
        emb.description = f"Successfully added {days} days to the license."
        await safe_send_followup(interaction, embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
    except Exception as e:
        logger.error(f"Error in comp command: {e}")
        await safe_send_followup(interaction, "Failed to add time to user.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="comp_all", description="Add time to all active users")
async def comp_all(interaction: discord.Interaction, days: int):
    await interaction.response.defer()
    if not await is_admin(interaction.user.id): return
    conn = get_db(interaction.guild_id); cur = conn.cursor()
    cur.execute("UPDATE users SET expiry_date = expiry_date + INTERVAL '%s days' WHERE is_banned = FALSE", (days,))
    conn.commit(); conn.close()
    emb = create_modern_embed("Global Compensation", guild_id=interaction.guild_id)
    emb.add_field(name="Time Added", value=f"`{days} Days`", inline=True)
    await interaction.followup.send(embed=emb)
    await dispatch_log(interaction.guild_id, emb)

@requires_config
@tree.command(name="pause", description="Freeze user license")
@app_commands.describe(member="The user to pause license for")
async def pause(interaction: discord.Interaction, member: discord.Member):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    try:
        cur = conn.cursor()
        cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_paused = TRUE, pause_started_at = NOW() WHERE discord_id = %s AND is_banned = FALSE AND is_paused = FALSE", (member.id,))
        affected = cur.rowcount
        conn.commit()
        
        if affected == 0:
            return await safe_send_followup(interaction, "User not found, already paused, or is banned.")
        
        emb = create_modern_embed("License Frozen", guild_id=interaction.guild_id)
        emb.add_field(name="User", value=member.mention, inline=True)
        emb.add_field(name="Login ID", value=f"```{member.id}```", inline=False)
        emb.add_field(name="Status", value="``PAUSED``", inline=True)
        emb.description = f"The license for @{member.display_name} has been frozen.\nTime will not count down while paused."
        await safe_send_followup(interaction, embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
    except Exception as e:
        logger.error(f"Error in pause command: {e}")
        await safe_send_followup(interaction, "Failed to pause license.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@requires_config
@tree.command(name="unpause", description="Resume paused user license")
@app_commands.describe(member="The user to resume license for")
async def unpause(interaction: discord.Interaction, member: discord.Member):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))

    try:
        cur = conn.cursor()
        cur.execute(f"SELECT pause_started_at, expiry_date FROM {get_table_prefix(interaction.guild_id)}_users WHERE discord_id = %s AND is_paused = TRUE", (member.id,))
        res = cur.fetchone()
        
        if not res:
            return await safe_send_followup(interaction, "User is not paused.")
        
        pause_start, expiry = res
        if pause_start:
            new_expiry = expiry + (datetime.now(pause_start.tzinfo) - pause_start)
            cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_paused = FALSE, expiry_date = %s, pause_started_at = NULL WHERE discord_id = %s", (new_expiry, member.id))
        else:
            cur.execute(f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_paused = FALSE, pause_started_at = NULL WHERE discord_id = %s", (member.id,))
        
        conn.commit()
        
        emb = create_modern_embed("License Resumed", guild_id=interaction.guild_id)
        emb.add_field(name="User", value=member.mention, inline=True)
        emb.add_field(name="Login ID", value=f"```{member.id}```", inline=False)
        emb.add_field(name="Status", value="``ACTIVE``", inline=True)
        emb.description = f"The license for @{member.display_name} has been resumed.\nTime countdown will continue normally."
        await safe_send_followup(interaction, embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
    except Exception as e:
        logger.error(f"Error in unpause command: {e}")
        await safe_send_followup(interaction, "Failed to resume license.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@tree.command(name="pause_all", description="Freeze all active licenses")
async def pause_all(interaction: discord.Interaction):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))
    
    try:
        cur = conn.cursor()
        cur.execute(
            f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_paused = TRUE, pause_started_at = NOW() WHERE is_banned = FALSE AND is_paused = FALSE"
        )
        count = cur.rowcount
        conn.commit()
        
        emb = create_modern_embed("Global License Freeze", guild_id=interaction.guild_id)
        emb.add_field(name="Status", value="All active users have been paused.", inline=False)
        emb.add_field(name="Affected", value=f"`{count} Users`", inline=True)
        await safe_send_followup(interaction, embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
    except Exception as e:
        logger.error(f"Error in pause_all command: {e}")
        await safe_send_followup(interaction, "Failed to pause all licenses.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@tree.command(name="unpause_all", description="Resume all paused licenses")
async def unpause_all(interaction: discord.Interaction):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        return await safe_send_followup(interaction, create_not_configured_embed(interaction.guild_id))
    
    try:
        cur = conn.cursor()
        cur.execute(
            f"SELECT discord_id, pause_started_at, expiry_date FROM {get_table_prefix(interaction.guild_id)}_users WHERE is_paused = TRUE"
        )
        paused_users = cur.fetchall()
        
        count = 0
        for uid, p_start, p_expiry in paused_users:
            if p_start:
                # Calculate time paused and extend expiry
                new_ex = p_expiry + (datetime.now(p_start.tzinfo) - p_start)
                cur.execute(
                    f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_paused = FALSE, expiry_date = %s, pause_started_at = NULL WHERE discord_id = %s",
                    (new_ex, uid)
                )
                count += 1
            else:
                # No pause start time, just unpause
                cur.execute(
                    f"UPDATE {get_table_prefix(interaction.guild_id)}_users SET is_paused = FALSE, pause_started_at = NULL WHERE discord_id = %s",
                    (uid,)
                )
                count += 1
        
        conn.commit()
        
        emb = create_modern_embed("Global License Resume", guild_id=interaction.guild_id)
        emb.add_field(name="Status", value="All users have been resumed.", inline=False)
        emb.add_field(name="Affected", value=f"`{count} Users`", inline=True)
        await safe_send_followup(interaction, embed=emb)
        
        # Non-blocking logging
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
    except Exception as e:
        logger.error(f"Error in unpause_all command: {e}")
        await safe_send_followup(interaction, "Failed to resume all licenses.")
    finally:
        release_db_connection(interaction.guild_id, conn)



@tree.command(name="setup", description="Setup system for this server")
async def setup(interaction: discord.Interaction):
    """Optimized setup command"""
    # Defer immediately to prevent timeout
    await interaction.response.defer()
    
    # Check if user is superadmin
    if not await is_superadmin_user(interaction.user.id):
        return await interaction.followup.send("❌ Only superadmins can use this command.", ephemeral=True)
    
    # Quick existing config check
    try:
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=5)
        cur = conn.cursor()
        cur.execute("SELECT guild_id FROM server_configs WHERE guild_id = %s", (interaction.guild_id,))
        if cur.fetchone():
            conn.close()
            return await interaction.followup.send("❌ This server is already configured!", ephemeral=True)
        conn.close()
    except Exception:
        return await interaction.followup.send("❌ Database error occurred.", ephemeral=True)
    
    # Get server info quickly
    guild = interaction.guild
    server_name = guild.name if guild else "Unknown Server"
    server_id = interaction.guild_id
    server_owner_mention = guild.owner.mention if guild and guild.owner else "@Unknown"
    
    # Database setup with optimized operations
    db_url = "postgresql://neondb_owner:npg_MDlhbV3k6PeA@ep-floral-water-ah029i1c-pooler.c-3.us-east-1.aws.neon.tech/neondb?sslmode=require"
    table_prefix = f"server_{server_id}"
    
    try:
        # Store configuration quickly
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=5)
        cur = conn.cursor()
        cur.execute("INSERT INTO server_configs (guild_id, db_url) VALUES (%s, %s)", (server_id, db_url))
        conn.commit()
        conn.close()
        
        # Create table quickly
        pool = psycopg2.pool.SimpleConnectionPool(2, 5, db_url, connect_timeout=5)
        db_conn = pool.getconn()
        db_cur = db_conn.cursor()
        
        db_cur.execute(f"""
            CREATE TABLE IF NOT EXISTS {table_prefix}_users (
                id SERIAL PRIMARY KEY,
                discord_id BIGINT UNIQUE,
                license_key TEXT UNIQUE,
                hwid TEXT,
                expiry_date TIMESTAMP,
                is_banned BOOLEAN DEFAULT FALSE,
                last_ip TEXT,
                reset_count INTEGER DEFAULT 0,
                created_at TIMESTAMP DEFAULT NOW(),
                is_paused BOOLEAN DEFAULT FALSE,
                pause_started_at TIMESTAMP,
                activated_at TIMESTAMP,
                duration_seconds INTEGER,
                CONSTRAINT user_identifier_check CHECK (discord_id IS NOT NULL OR license_key IS NOT NULL)
            )
        """)
        
        db_conn.commit()
        pool.putconn(db_conn)
        
        # Create embed immediately with correct field order
        emb = create_modern_embed("Server Setup Complete", guild_id=interaction.guild_id, color=0xFFFFFF)
        emb.description = f"**{server_name}** system is now ready!"
        emb.add_field(name="Server Owner", value=server_owner_mention, inline=True)
        emb.add_field(name="Server Name", value=f"`{server_name}`", inline=True)
        emb.add_field(name="Setup By", value=interaction.user.mention, inline=True)
        emb.add_field(name="Database", value="✅ Connected", inline=True)
        emb.add_field(name="Tables", value="✅ Created", inline=True)
        
        await interaction.followup.send(embed=emb)
        logger.info(f"Server {server_name} ({server_id}) setup completed by {interaction.user}")
        
        # Quick command sync
        try:
            await tree.sync(guild=guild)
        except Exception:
            pass  # Don't fail if sync fails
        
    except Exception as e:
        logger.error(f"Setup error: {e}")
        await interaction.followup.send("❌ Setup failed.", ephemeral=True)

@tree.command(name="link", description="Link this server to an existing database from another server")
@app_commands.describe(
    guild_id="The guild ID of the server to link from"
)
async def link(interaction: discord.Interaction, guild_id: str):
    """Link current server to existing database from another server"""
    # Defer immediately to prevent timeout
    await interaction.response.defer()
    
    # Check if user is superadmin
    if not await is_superadmin_user(interaction.user.id):
        return await interaction.followup.send("❌ Only superadmins can use this command.", ephemeral=True)
    
    # Validate guild_id format
    try:
        target_guild_id = int(guild_id)
        if not validate_user_id(target_guild_id):
            return await interaction.followup.send("❌ Invalid guild ID format.", ephemeral=True)
    except ValueError:
        return await interaction.followup.send("❌ Guild ID must be a number.", ephemeral=True)
    
    # Check if current server is already configured
    try:
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=5)
        cur = conn.cursor()
        cur.execute("SELECT guild_id FROM server_configs WHERE guild_id = %s", (interaction.guild_id,))
        if cur.fetchone():
            conn.close()
            return await interaction.followup.send("❌ This server is already configured! Use /unlink first if you want to relink.", ephemeral=True)
        conn.close()
    except Exception:
        return await interaction.followup.send("❌ Database error occurred.", ephemeral=True)
    
    try:
        # Get database URL from target server
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=5)
        cur = conn.cursor()
        cur.execute("SELECT db_url FROM server_configs WHERE guild_id = %s", (target_guild_id,))
        result = cur.fetchone()
        conn.close()
        
        if not result:
            return await interaction.followup.send("❌ Target server not found or not configured.", ephemeral=True)
        
        db_url = result[0]
        
        # Test the database connection
        try:
            test_conn = psycopg2.connect(db_url, connect_timeout=5)
            test_cur = test_conn.cursor()
            test_cur.execute("SELECT 1")
            test_cur.close()
            test_conn.close()
        except Exception as e:
            logger.error(f"Database connection test failed: {e}")
            return await interaction.followup.send("❌ Failed to connect to target database.", ephemeral=True)
        
        # Store configuration for current server
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=5)
        cur = conn.cursor()
        cur.execute("INSERT INTO server_configs (guild_id, db_url, linked_from_guild_id) VALUES (%s, %s, %s)", 
                   (interaction.guild_id, db_url, target_guild_id))
        conn.commit()
        conn.close()
        
        # Get server info
        guild = interaction.guild
        server_name = guild.name if guild else "Unknown Server"
        
        # Create success embed
        emb = create_modern_embed("Server Linked Successfully", guild_id=interaction.guild_id, color=0xFFFFFF)
        emb.description = f"**{server_name}** is now linked to the database from server `{target_guild_id}`!"
        emb.add_field(name="Current Server", value=f"`{server_name}` ({interaction.guild_id})", inline=True)
        emb.add_field(name="Linked To", value=f"Server `{target_guild_id}`", inline=True)
        emb.add_field(name="Linked By", value=interaction.user.mention, inline=True)
        emb.add_field(name="Database", value="✅ Shared", inline=True)
        emb.add_field(name="Logging", value="✅ Shared", inline=True)
        emb.add_field(name="Note", value="Both servers now share the same user database and logging channel. All actions in this server will log to the original server's logging channel.", inline=False)
        
        await interaction.followup.send(embed=emb)
        logger.info(f"Server {server_name} ({interaction.guild_id}) linked to database from {target_guild_id} by {interaction.user}")
        
        # Quick command sync
        try:
            await tree.sync(guild=guild)
        except Exception:
            pass  # Don't fail if sync fails
        
    except Exception as e:
        logger.error(f"Link error: {e}")
        await interaction.followup.send("❌ Failed to link server.", ephemeral=True)

@tree.command(name="unlink", description="Unlink this server from any database")
async def unlink(interaction: discord.Interaction):
    """Unlink current server from database"""
    # Defer immediately to prevent timeout
    await interaction.response.defer()
    
    # Check if user is superadmin
    if not await is_superadmin_user(interaction.user.id):
        return await interaction.followup.send("❌ Only superadmins can use this command.", ephemeral=True)
    
    try:
        # Remove server configuration
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=5)
        cur = conn.cursor()
        cur.execute("DELETE FROM server_configs WHERE guild_id = %s", (interaction.guild_id,))
        cur.execute("DELETE FROM logging_config WHERE guild_id = %s", (interaction.guild_id,))
        conn.commit()
        conn.close()
        
        # Clear connection pool
        if str(interaction.guild_id) in db_pools:
            try:
                db_pools[str(interaction.guild_id)].closeall()
                del db_pools[str(interaction.guild_id)]
            except Exception:
                pass
        
        # Get server info
        guild = interaction.guild
        server_name = guild.name if guild else "Unknown Server"
        
        # Create success embed
        emb = create_modern_embed("Server Unlinked", guild_id=interaction.guild_id, color=0xFFFFFF)
        emb.description = f"**{server_name}** has been unlinked from the database."
        emb.add_field(name="Server", value=f"`{server_name}` ({interaction.guild_id})", inline=True)
        emb.add_field(name="Unlinked By", value=interaction.user.mention, inline=True)
        emb.add_field(name="Status", value="✅ Disconnected", inline=True)
        emb.add_field(name="Note", value="Run /link to connect to another database or /setup to create a new one.", inline=False)
        
        await interaction.followup.send(embed=emb)
        logger.info(f"Server {server_name} ({interaction.guild_id}) unlinked by {interaction.user}")
        
    except Exception as e:
        logger.error(f"Unlink error: {e}")
        await interaction.followup.send("❌ Failed to unlink server.", ephemeral=True)

@tree.command(name="serverid", description="Get the current server's guild ID")
async def serverid(interaction: discord.Interaction):
    """Display the current server's guild ID"""
    # Get server info
    guild = interaction.guild
    server_name = guild.name if guild else "Unknown Server"
    
    # Create embed with server ID
    emb = create_modern_embed("Server Information", guild_id=interaction.guild_id, color=0xFFFFFF)
    emb.description = f"**{server_name}** Server Information"
    emb.add_field(name="Server Name", value=f"`{server_name}`", inline=True)
    emb.add_field(name="Guild ID", value=f"```{interaction.guild_id}```", inline=True)
    emb.add_field(name="Copy Command", value=f"```/link guild_id:{interaction.guild_id}```", inline=False)
    emb.add_field(name="Note", value="Use this Guild ID to link other servers to this server's database.", inline=False)
    
    await interaction.response.send_message(embed=emb, ephemeral=True)

@tree.command(name="migrate_db", description="Update database schema for new features")
async def migrate_db(interaction: discord.Interaction):
    """Migrate database to support license keys and linking functionality"""
    # Only system owner can use this
    if interaction.user.id != SEBWETT_ID:
        return await interaction.response.send_message("❌ Only the system owner can use this command.", ephemeral=True)
    
    await interaction.response.defer()
    
    migrations_applied = []
    
    try:
        # 1. Migrate server_configs table
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
        cur = conn.cursor()
        
        # Check for linked_from_guild_id column
        cur.execute("""
            SELECT column_name 
            FROM information_schema.columns 
            WHERE table_name = 'server_configs' 
            AND column_name = 'linked_from_guild_id'
        """)
        
        if not cur.fetchone():
            cur.execute("ALTER TABLE server_configs ADD COLUMN linked_from_guild_id BIGINT")
            conn.commit()
            migrations_applied.append("✅ Added `linked_from_guild_id` to server_configs")
        
        # Check for license_key_format column
        cur.execute("""
            SELECT column_name 
            FROM information_schema.columns 
            WHERE table_name = 'server_configs' 
            AND column_name = 'license_key_format'
        """)
        
        if not cur.fetchone():
            cur.execute("ALTER TABLE server_configs ADD COLUMN license_key_format TEXT")
            conn.commit()
            migrations_applied.append("✅ Added `license_key_format` to server_configs")
        
        # 2. Migrate user tables for this server
        table_prefix = get_table_prefix(str(interaction.guild_id))
        
        # Get database connection for this server
        db_conn = get_db_connection(interaction.guild_id)
        if db_conn:
            db_cur = db_conn.cursor()
            
            # Check if license_key column exists
            db_cur.execute(f"""
                SELECT column_name 
                FROM information_schema.columns 
                WHERE table_name = '{table_prefix}_users' 
                AND column_name = 'license_key'
            """)
            
            if not db_cur.fetchone():
                # Check if table has old schema (discord_id as primary key)
                db_cur.execute(f"""
                    SELECT column_name 
                    FROM information_schema.columns 
                    WHERE table_name = '{table_prefix}_users' 
                    AND column_name = 'id'
                """)
                
                has_id_column = db_cur.fetchone() is not None
                
                if not has_id_column:
                    # Old schema - need to add id column and restructure
                    # Add id column
                    db_cur.execute(f"ALTER TABLE {table_prefix}_users ADD COLUMN id SERIAL")
                    
                    # Drop old primary key constraint
                    db_cur.execute(f"ALTER TABLE {table_prefix}_users DROP CONSTRAINT {table_prefix}_users_pkey")
                    
                    # Set id as new primary key
                    db_cur.execute(f"ALTER TABLE {table_prefix}_users ADD PRIMARY KEY (id)")
                    
                    # Make discord_id unique but not primary key
                    db_cur.execute(f"ALTER TABLE {table_prefix}_users ADD CONSTRAINT {table_prefix}_users_discord_id_key UNIQUE (discord_id)")
                    
                    migrations_applied.append(f"✅ Restructured `{table_prefix}_users` with new id column")
                
                # Now add license_key column
                db_cur.execute(f"ALTER TABLE {table_prefix}_users ADD COLUMN license_key TEXT UNIQUE")
                
                # Make discord_id nullable
                db_cur.execute(f"ALTER TABLE {table_prefix}_users ALTER COLUMN discord_id DROP NOT NULL")
                
                # Add constraint to ensure at least one identifier exists
                db_cur.execute(f"""
                    ALTER TABLE {table_prefix}_users 
                    ADD CONSTRAINT user_identifier_check 
                    CHECK (discord_id IS NOT NULL OR license_key IS NOT NULL)
                """)
                
                db_conn.commit()
                migrations_applied.append(f"✅ Added license key support to `{table_prefix}_users`")
            
            # Check for activated_at column
            db_cur.execute(f"""
                SELECT column_name 
                FROM information_schema.columns 
                WHERE table_name = '{table_prefix}_users' 
                AND column_name = 'activated_at'
            """)
            
            if not db_cur.fetchone():
                db_cur.execute(f"ALTER TABLE {table_prefix}_users ADD COLUMN activated_at TIMESTAMP")
                db_conn.commit()
                migrations_applied.append(f"✅ Added `activated_at` to `{table_prefix}_users`")
            
            # Check for duration_seconds column
            db_cur.execute(f"""
                SELECT column_name 
                FROM information_schema.columns 
                WHERE table_name = '{table_prefix}_users' 
                AND column_name = 'duration_seconds'
            """)
            
            if not db_cur.fetchone():
                db_cur.execute(f"ALTER TABLE {table_prefix}_users ADD COLUMN duration_seconds INTEGER")
                db_conn.commit()
                migrations_applied.append(f"✅ Added `duration_seconds` to `{table_prefix}_users`")
            
            release_db_connection(interaction.guild_id, db_conn)
        
        conn.close()
        
        # Create response embed
        if migrations_applied:
            emb = create_modern_embed("Database Migration Complete", guild_id=interaction.guild_id)
            emb.description = "\n".join(migrations_applied)
            emb.add_field(name="Status", value="✅ Migration successful", inline=False)
        else:
            emb = create_modern_embed("Database Schema Updated", guild_id=interaction.guild_id)
            emb.description = "✅ Database schema is already up to date"
            emb.add_field(name="Status", value="✅ No migration needed", inline=False)
        
        await interaction.followup.send(embed=emb)
        
    except Exception as e:
        logger.error(f"Migration error: {e}")
        emb = create_modern_embed("Migration Error", guild_id=interaction.guild_id)
        emb.description = f"❌ Migration failed: {str(e)}"
        await interaction.followup.send(embed=emb, ephemeral=True)

@tree.command(name="logging", description="Set up the logging channel for bot events")
async def logging_cmd(interaction: discord.Interaction):
    """Optimized logging command that uses the current channel"""
    # Quick system owner check
    if interaction.user.id != SEBWETT_ID and not await is_admin(interaction.user.id):
        return await interaction.response.send_message("❌ Only administrators can use this command.", ephemeral=True)
    
    # Quick channel check
    if not isinstance(interaction.channel, discord.TextChannel):
        return await interaction.response.send_message("❌ This command can only be used in a text channel.", ephemeral=True)
    
    # Defer immediately
    await interaction.response.defer()
    
    # Parallel database operations
    try:
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=5)
        cur = conn.cursor()
        cur.execute(
            "INSERT INTO logging_config (guild_id, channel_id) VALUES (%s, %s) "
            "ON CONFLICT(guild_id) DO UPDATE SET channel_id = %s",
            (interaction.guild_id, interaction.channel.id, interaction.channel.id)
        )
        conn.commit()
        conn.close()
        
        # Create embed immediately
        emb = create_modern_embed("Logging Channel Configured", guild_id=interaction.guild_id, color=0xFFFFFF)
        emb.description = f"All bot events will now be logged to {interaction.channel.mention}"
        emb.add_field(name="Channel", value=interaction.channel.mention, inline=True)
        emb.add_field(name="Configured By", value=interaction.user.mention, inline=True)
        
        await interaction.followup.send(embed=emb)
        logger.info(f"Logging configured for guild {interaction.guild_id} in channel {interaction.channel.id}")
        
    except Exception as e:
        logger.error(f"Error configuring logging: {e}")
        await interaction.followup.send("❌ Failed to configure logging channel.", ephemeral=True)

@tree.command(name="stats", description="Database health overview")
async def stats(interaction: discord.Interaction):
    # Quick validation first
    if not await is_admin(interaction.user.id):
        return await interaction.response.send_message("Unauthorized.", ephemeral=True)
    
    # Check if database is configured first
    conn = get_db_connection(interaction.guild_id)
    if not conn:
        def create_not_configured_embed(guild_id: int, color: int = 0xFF6B6B, title: str = "🚀 Setup Required!"):
            """Create a fun 'not configured' embed"""
            emb = create_modern_embed(title, guild_id=guild_id, color=color)
            emb.description = "🚀 This server needs some love! 💫\n\nRun /setup to unlock full power! 🎯"
            emb.add_field(name="🔑 Required Command", value="`/setup` (System owner only)", inline=True)
            emb.add_field(name="📋 Quick Start Guide", value="1️⃣ Run `/setup` to create database\n2️⃣ Configure logging with `/logging`\n3️⃣ Start managing users like a pro! 🚀", inline=False)
            return emb
        emb = create_not_configured_embed(interaction.guild_id)
        return await interaction.response.send_message(embed=emb, ephemeral=True)
    
    # Defer only after validation
    await interaction.response.defer()
    
    try:
        cur = conn.cursor()
        
        # Get comprehensive statistics
        cur.execute(f"""
            SELECT 
                COUNT(*) as total_users,
                COUNT(*) FILTER (WHERE is_banned = TRUE) as banned_users,
                COUNT(*) FILTER (WHERE is_paused = TRUE) as paused_users,
                COUNT(*) FILTER (WHERE expiry_date > NOW() AND is_banned = FALSE AND is_paused = FALSE) as active_users,
                COUNT(*) FILTER (WHERE expiry_date <= NOW()) as expired_users,
                COUNT(*) FILTER (WHERE hwid IS NOT NULL) as hwid_locked_users,
                COUNT(*) FILTER (WHERE license_key IS NOT NULL) as license_key_users,
                COUNT(*) FILTER (WHERE discord_id IS NOT NULL AND license_key IS NULL) as discord_only_users
            FROM {get_table_prefix(interaction.guild_id)}_users
        """)
        
        stats_row = cur.fetchone()
        total, banned, paused, active, expired, hwid_locked, license_users, discord_users = stats_row
        
        # Calculate inactive users (not banned, not paused, but expired)
        inactive = expired - banned - paused
        
        emb = create_modern_embed("Database Statistics", guild_id=interaction.guild_id)
        emb.description = "Comprehensive overview of your license database"
        
        # User Status
        emb.add_field(name="Total Users", value=f"``{total}``", inline=True)
        emb.add_field(name="Active Users", value=f"``{active}``", inline=True)
        emb.add_field(name="Expired Users", value=f"``{expired}``", inline=True)
        
        # User States
        emb.add_field(name="Banned Users", value=f"``{banned}``", inline=True)
        emb.add_field(name="Paused Users", value=f"``{paused}``", inline=True)
        emb.add_field(name="HWID Locked", value=f"``{hwid_locked}``", inline=True)
        
        # User Types
        emb.add_field(name="License Key Users", value=f"``{license_users}``", inline=True)
        emb.add_field(name="Discord ID Users", value=f"``{discord_users}``", inline=True)
        emb.add_field(name="Health Status", value=f"``{'✓ Healthy' if active > 0 else '⚠ No Active Users'}``", inline=True)
        
        await safe_send_followup(interaction, embed=emb)
        
    except Exception as e:
        logger.error(f"Error in stats command: {e}")
        await safe_send_followup(interaction, "Failed to fetch stats.")
    finally:
        release_db_connection(interaction.guild_id, conn)

@tree.command(name="help", description="Get setup instructions and command help")
async def help_cmd(interaction: discord.Interaction):
    # Check if database is configured
    conn = get_db_connection(interaction.guild_id)
    is_configured = conn is not None
    if conn:
        release_db_connection(interaction.guild_id, conn)
    
    emb = create_modern_embed("System Bot Help", guild_id=interaction.guild_id)
    
    if not is_configured:
        emb.description = "**Setup Required** - This server needs configuration!"
        emb.add_field(
            name="Quick Setup", 
            value="1. /setup - Create database (System owner only)\n2. /logging - Set up logging channel\n3. /admin_add - Add bot admins", 
            inline=False
        )
        emb.add_field(
            name="Why setup?", 
            value="The bot needs a database to store user licenses, HWID locks, and other data. Without setup, most commands won't work.", 
            inline=False
        )
    else:
        emb.description = "**Bot is configured** - All commands available!"
        emb.add_field(
            name="User Management", 
            value="/gen - Create/extend license\n/ban - Blacklist user\n/unban - Remove blacklist\n/profile - View user info\n/reset - Clear HWID lock", 
            inline=True
        )
        emb.add_field(
            name="License Control", 
            value="/pause - Freeze user license\n/unpause - Resume license\n/pause_all - Freeze all licenses\n/unpause_all - Resume all", 
            inline=True
        )
        emb.add_field(
            name="System Commands", 
            value="/stats - Database overview\n/comp_all - Add time to all users\n/comp - Add time to specific user\n/csharp - Get C# loader\n/cpp - Get C++ loader", 
            inline=True
        )
    
    emb.add_field(
        name="Admin Only", 
        value="/admin_add - Add bot admin\n/admin_remove - Remove admin\n/admin_list - List admins", 
        inline=True
    )
    
    emb.add_field(
        name="API Info", 
        value="Login API: POST /log_login\nPort: 11542\nFull docs in /csharp command output", 
        inline=False
    )
    
    emb.set_footer(text="Need help? Contact the system owner for support!")
    await interaction.response.send_message(embed=emb, ephemeral=True)

@app.post("/loader_opened")
async def loader_opened(request: Request):
    """Track when users open the loader"""
    try:
        data = await request.json()
        
        guild_id = str(data.get("guild_id", ""))
        user_id = data.get("user_id")
        ip = request.client.host or "0.0.0.0"
        
        # Log the loader opened event
        await log_loader_event(int(guild_id), int(user_id), "loader_opened", ip=ip)
        
        return {"status": "success", "message": "Loader opened event logged"}
        
    except Exception as e:
        logger.error(f"API Error in loader_opened: {e}")
        return {"status": "error", "message": "Internal server error"}

@app.post("/log_login")
async def log_login(request: Request):
    try:
        # Get raw body first for debugging
        raw_body = await request.body()
        logger.info(f"[LOGIN ATTEMPT] Raw body: {raw_body.decode('utf-8', errors='replace')}")
        
        # Try to parse JSON with better error handling
        try:
            data = await request.json()
        except json.JSONDecodeError as json_err:
            logger.error(f"[LOGIN ERROR] JSON parsing failed: {json_err}")
            logger.error(f"[LOGIN ERROR] Raw body was: {raw_body.decode('utf-8', errors='replace')}")
            return {"status": "error", "message": "Invalid JSON format"}
        
        logger.info(f"[LOGIN ATTEMPT] Received data: {data}")
        
        # Input validation - require guild_id and either user_id OR license_key
        guild_id = str(data.get("guild_id", ""))
        # Strip leading zeros from guild_id (e.g., "0836783046329237514" -> "836783046329237514")
        # This ensures database table names match correctly
        guild_id = guild_id.lstrip("0") if guild_id else ""
        user_id = data.get("user_id")  # Could be Discord ID or license key
        license_key = data.get("license_key")  # License key (optional)
        ip = request.client.host or "0.0.0.0"
        
        logger.info(f"[LOGIN ATTEMPT] Parsed - Guild: {guild_id}, User: {user_id}, License: {license_key}, IP: {ip}")
        
        # Validate inputs
        if not guild_id:
            logger.warning(f"[LOGIN FAILED] Missing guild_id")
            return {"status": "error", "message": "Missing required field: guild_id"}
        
        if not user_id and not license_key:
            logger.warning(f"[LOGIN FAILED] Missing both user_id and license_key")
            return {"status": "error", "message": "Missing required field: user_id or license_key"}
        
        # CHECK IP BAN FIRST - before any other checks
        try:
            master_conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=5)
            master_cur = master_conn.cursor()
            master_cur.execute(
                "SELECT reason FROM ip_bans WHERE guild_id = %s AND ip_address = %s",
                (int(guild_id), ip)
            )
            ip_ban_result = master_cur.fetchone()
            master_conn.close()
            
            if ip_ban_result:
                logger.warning(f"[LOGIN FAILED] Banned IP {ip} attempted login for guild {guild_id}")
                # Determine identifier for logging
                identifier = user_id if user_id else license_key
                await log_loader_event(int(guild_id), identifier, "login_failed", ip=ip, additional_info=f"IP Banned: {ip_ban_result[0]}")
                return {"status": "error", "message": "[-] You are banned from this loader"}
        except Exception as e:
            logger.error(f"[IP BAN CHECK ERROR] {e}")
            # Continue with login if IP ban check fails (don't block legitimate users)
        
        # Determine if user_id is a Discord ID or license key
        is_discord_id = False
        if user_id:
            # Try to parse as integer (Discord ID)
            try:
                user_id_int = int(user_id)
                if validate_user_id(user_id_int):
                    is_discord_id = True
                    identifier_for_log = user_id_int
                else:
                    # Not a valid Discord ID, treat as license key
                    license_key = user_id
                    user_id = None
                    identifier_for_log = license_key
            except (ValueError, TypeError):
                # Not an integer, treat as license key
                license_key = user_id
                user_id = None
                identifier_for_log = license_key
        else:
            identifier_for_log = license_key
            
        logger.info(f"[LOGIN PROCESS] Getting database connection for guild {guild_id}")
        
        # Run blocking database operations in thread pool
        loop = asyncio.get_event_loop()
        conn = await loop.run_in_executor(db_executor, get_db_connection, guild_id)
        if not conn:
            logger.error(f"[LOGIN FAILED] Database not configured for guild {guild_id}")
            identifier = user_id if user_id else license_key
            await log_loader_event(int(guild_id), identifier, "login_failed", ip=ip, additional_info="Database not configured")
            return {"status": "error", "message": "Database not configured"}

        cur = conn.cursor()
        try:
            # Check if user exists by Discord ID or license key
            if user_id:
                logger.info(f"[LOGIN QUERY] Checking Discord user {user_id} in guild {guild_id}")
                cur.execute(
                    f"SELECT discord_id, license_key, hwid, is_banned, expiry_date, is_paused, activated_at, duration_seconds FROM {get_table_prefix(guild_id)}_users WHERE discord_id = %s",
                    (int(user_id),)
                )
                identifier_for_log = user_id
            else:
                logger.info(f"[LOGIN QUERY] Checking license key {license_key} in guild {guild_id}")
                cur.execute(
                    f"SELECT discord_id, license_key, hwid, is_banned, expiry_date, is_paused, activated_at, duration_seconds FROM {get_table_prefix(guild_id)}_users WHERE license_key = %s",
                    (license_key,)
                )
                identifier_for_log = license_key
            
            res = cur.fetchone()
            logger.info(f"[LOGIN RESULT] Database query result: {res}")

            if not res:
                logger.warning(f"[LOGIN FAILED] Identifier {identifier_for_log} not found in database")
                await log_loader_event(int(guild_id), identifier_for_log, "login_failed", ip=ip, additional_info="User not found in database")
                return {"status": "error", "message": "User not found in database"}

            discord_id, lic_key, db_hwid, is_banned, expiry, is_paused, activated_at, duration_seconds = res
            logger.info(f"[LOGIN STATUS] User data - Discord: {discord_id}, License: {lic_key}, HWID: {db_hwid}, Banned: {is_banned}, Paused: {is_paused}, Expiry: {expiry}, Activated: {activated_at}, Duration: {duration_seconds}")

            # Handle first-time activation for license keys
            if activated_at is None and duration_seconds is not None:
                # This is the first activation - calculate expiry from now
                activated_at = datetime.now()
                expiry = activated_at + timedelta(seconds=duration_seconds)
                
                # Update the database with activation time and expiry
                if user_id:
                    cur.execute(
                        f"UPDATE {get_table_prefix(guild_id)}_users SET activated_at = %s, expiry_date = %s WHERE discord_id = %s",
                        (activated_at, expiry, int(user_id))
                    )
                else:
                    cur.execute(
                        f"UPDATE {get_table_prefix(guild_id)}_users SET activated_at = %s, expiry_date = %s WHERE license_key = %s",
                        (activated_at, expiry, license_key)
                    )
                conn.commit()
                logger.info(f"[FIRST ACTIVATION] License activated for {identifier_for_log}, expiry set to {expiry}")

            if is_banned:
                logger.warning(f"[LOGIN FAILED] Banned user {identifier_for_log} attempted login from {ip}")
                await log_loader_event(int(guild_id), identifier_for_log, "login_failed", ip=ip, additional_info="Account blacklisted - Contact support")
                return {"status": "error", "message": "Account blacklisted - Contact support"}
            
            if is_paused:
                logger.warning(f"[LOGIN FAILED] Paused user {identifier_for_log} attempted login from {ip}")
                await log_loader_event(int(guild_id), identifier_for_log, "login_failed", ip=ip, additional_info="License Paused")
                return {"status": "error", "message": "License Paused"}

            if expiry and expiry < datetime.now():
                logger.warning(f"[LOGIN FAILED] Expired user {identifier_for_log} attempted login from {ip}")
                await log_loader_event(int(guild_id), identifier_for_log, "login_failed", ip=ip, additional_info="License expired - Renew required")
                return {"status": "error", "message": "License expired - Renew required"}

            # Get MachineSID from client (HWID)
            client_hwid = data.get("hwid") or data.get("machine_sid")
            
            if not client_hwid:
                logger.warning(f"[LOGIN FAILED] No HWID/MachineSID provided by client for user {identifier_for_log}")
                await log_loader_event(int(guild_id), identifier_for_log, "login_failed", ip=ip, additional_info="No MachineSID provided")
                return {"status": "error", "message": "Hardware identification required"}
            
            # Sanitize the HWID (remove any special characters, limit length)
            client_hwid = validate_input(str(client_hwid), max_length=128)
            if not client_hwid:
                logger.warning(f"[LOGIN FAILED] Invalid HWID format for user {identifier_for_log}")
                await log_loader_event(int(guild_id), identifier_for_log, "login_failed", ip=ip, additional_info="Invalid MachineSID format")
                return {"status": "error", "message": "Invalid hardware identification"}
            
            logger.info(f"[LOGIN HWID] Client provided MachineSID: {client_hwid}")
            
            # HWID Lock Logic
            if db_hwid is None:
                # First time logging in, lock the MachineSID
                if user_id:
                    cur.execute(f"UPDATE {get_table_prefix(guild_id)}_users SET hwid = %s, last_ip = %s WHERE discord_id = %s", (client_hwid, ip, int(user_id)))
                else:
                    cur.execute(f"UPDATE {get_table_prefix(guild_id)}_users SET hwid = %s, last_ip = %s WHERE license_key = %s", (client_hwid, ip, license_key))
                conn.commit()
                logger.info(f"[LOGIN SUCCESS] MachineSID registered for user {identifier_for_log}: {client_hwid}")
            elif db_hwid != client_hwid:
                logger.warning(f"[LOGIN FAILED] MachineSID mismatch for user {identifier_for_log}: expected {db_hwid}, got {client_hwid}")
                await log_loader_event(int(guild_id), identifier_for_log, "login_failed", hwid=client_hwid, ip=ip, additional_info="Device authorization failed - MachineSID mismatch")
                return {"status": "error", "message": "Device authorization failed - MachineSID mismatch"}

            # Success - Enhanced logging
            logger.info(f"[LOGIN SUCCESS] User {identifier_for_log} authenticated successfully from {ip}")
            await log_loader_event(int(guild_id), identifier_for_log, "login_success", hwid=client_hwid, ip=ip)

            # Convert expiry datetime to Unix timestamp for the loader
            expiry_timestamp = int(expiry.timestamp())
            
            return {"status": "success", "message": "Access granted", "expiry": expiry_timestamp}

        except Exception as e:
            logger.error(f"[LOGIN ERROR] Database error in log_login: {e}")
            identifier = user_id if user_id else license_key
            await log_loader_event(int(guild_id), identifier, "login_failed", ip=ip, additional_info=f"Database error: {str(e)}")
            return {"status": "error", "message": "Database operation failed"}
        finally:
            release_db_connection(guild_id, conn)

    except Exception as e:
        logger.error(f"[LOGIN ERROR] API Error in log_login: {e}")
        return {"status": "error", "message": "Internal server error"}
@tree.command(name="csharp", description="C# Loader (Legacy Compiler Support)")
async def csharp(interaction: discord.Interaction, api_ip: str = "217.154.173.102:11542"):
    await interaction.response.defer()
    guild_id = str(interaction.guild_id)
    
    # Format URL correctly
    clean_ip = api_ip.replace("http://", "").replace("https://", "").rstrip('/')
    final_url = f"http://{clean_ip}/log_login"
    
    # Rewritten for C# 7.3+ compatibility (Clean Client-Side)
    code = f"""using System;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace SystemLoader
{{
    class Program
    {{
        private static readonly HttpClient client = new HttpClient();

        static void Main(string[] args)
        {{
            // Entry point calls the Async Task
            RunLoader().GetAwaiter().GetResult();
        }}

        static async Task RunLoader()
        {{
            Console.Title = "System Gateway";
            
            Console.Write("[+] Enter your user ID: ");
            string uid = Console.ReadLine()?.Trim();

            string targetUrl = "{final_url}";
            string gId = "{guild_id}";

            Console.WriteLine("\\n[-] Authenticating...");
            
            try
            {{
                // Simple JSON payload - only guild_id and user_id needed
                string json = "{{\\"guild_id\\": " + gId + ", \\"user_id\\": \\"" + uid + "\\"}}";
                var content = new StringContent(json, Encoding.UTF8, "application/json");
                
                var response = await client.PostAsync(targetUrl, content);
                string result = await response.Content.ReadAsStringAsync();

                // Server handles all status codes and error messages
                if (response.IsSuccessStatusCode)
                {{
                    if (result.Contains("\\"status\\":\\"success\\"") || result.Contains("\\"status\\": \\"success\\""))
                    {{
                        Console.ForegroundColor = ConsoleColor.Green;
                        Console.WriteLine("\\n[*] Welcome to your loader.");
                        Console.ResetColor();
                        
                        // Your loader code starts here
                        Console.WriteLine("// The rest of your code...");
                        Console.ReadKey();
                    }}
                    else
                    {{
                        Console.ForegroundColor = ConsoleColor.Red;
                        // Server provides clean error message
                        string errorMsg = "Access denied";
                        if (result.Contains("\\\"message\\\":"))
                        {{
                            int start = result.IndexOf("\\\"message\\\":\\\"") + 11;
                            int end = result.IndexOf("\\\"", start);
                            if (end > start)
                            {{
                                errorMsg = result.Substring(start, end - start);
                            }}
                        }}
                        Console.WriteLine("\\n[*] " + errorMsg);
                        Console.ResetColor();
                        Environment.Exit(0);
                    }}
                }}
                else
                {{
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("\\n[*] Server connection error");
                    Console.ResetColor();
                    Environment.Exit(0);
                }}
            }}
            catch (Exception ex)
            {{
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("\\n[*] Connection failed: " + ex.Message);
                Console.ResetColor();
                Environment.Exit(0);
            }}
        }}
    }}
}}"""
    file_bytes = io.BytesIO(code.encode('utf-8'))
    discord_file = discord.File(fp=file_bytes, filename="Loader.cs")
    await interaction.followup.send(embed=create_modern_embed("C# Classic Source"), file=discord_file)

@tree.command(name="sync_commands", description="Force sync all commands to Discord")
async def sync_commands(interaction: discord.Interaction):
    if interaction.user.id != SEBWETT_ID:
        return await interaction.response.send_message("Forbidden.", ephemeral=True)
    
    await interaction.response.defer()
    
    try:
        synced = await tree.sync()
        await interaction.followup.send(f"✅ Commands synced successfully! Synced {len(synced)} commands.")
        logger.info("Manual command sync completed")
    except Exception as e:
        await interaction.followup.send(f"❌ Sync failed: {e}")
        logger.error(f"Manual sync failed: {e}")

@tree.command(name="test_sync", description="Test if commands are working")
async def test_sync(interaction: discord.Interaction):
    if interaction.user.id != SEBWETT_ID:
        return await interaction.response.send_message("Forbidden.", ephemeral=True)
    
    # Just test if we can respond
    await interaction.response.send_message("✅ Bot is responding to commands! Try `/sync_commands` now.")

@tree.command(name="cpp", description="C++ Loader with Character Fix")
async def cpp(interaction: discord.Interaction, api_ip: str = "localhost"):
    await interaction.response.defer()
    guild_id = str(interaction.guild_id)
    
    if ":" in api_ip:
        clean_ip, clean_port = api_ip.split(":")
    else:
        clean_ip, clean_port = api_ip, "8000"
    
    code = r"""#include <iostream>
#include <windows.h>
#include <string>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

using namespace std;

int main() {
    system("title SebwettSQL C++ Gateway");
    
    string uid;
    cout << "[+] Enter your user ID: ";
    cin >> uid;

    cout << endl;
    cout << "[-] Authenticating...";

    HINTERNET hSession = InternetOpenA("Loader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hSession) {
        HINTERNET hConnect = InternetConnectA(hSession, "{IP_HERE}", {PORT_HERE}, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (hConnect) {
            HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", "/log_login", NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);
            
            // Simple JSON payload - only guild_id and user_id needed
            string data = "{\"guild_id\": {GID}, \"user_id\": \"" + uid + "\"}";
            
            bool sent = HttpSendRequestA(hRequest, "Content-Type: application/json", -1, (LPVOID)data.c_str(), (DWORD)data.length());
            
            if (sent) {
                char buffer[2048];
                DWORD bytesRead;
                InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead);
                buffer[bytesRead] = '\0';
                string result = string(buffer);

                cout << "\r" << string(50, ' ') << "\r";

                // Server handles all status codes and error messages
                if (result.find("\"status\":\"success\"") != string::npos || result.find("\"status\": \"success\"") != string::npos) {
                    system("color 0A");
                    cout << "\n[*] Welcome to your loader." << endl;
                    
                    // Your loader code starts here
                    cout << "// The rest of your code..." << endl;
                    system("pause > nul");
                } 
                else {
                    system("color 0C");
                    // Server provides clean error message
                    string errorMsg = "[*] Access denied";
                    size_t msgStart = result.find("\"message\":\"");
                    if (msgStart != string::npos) {
                        msgStart += 11;
                        size_t msgEnd = result.find("\"", msgStart);
                        if (msgEnd != string::npos) {
                            errorMsg = result.substr(msgStart, msgEnd - msgStart);
                        }
                    }
                    cout << "\n[*] " << errorMsg << endl;
                    Sleep(1500); 
                    return 0;
                }
            }
            InternetCloseHandle(hRequest);
        }
        InternetCloseHandle(hConnect);
    }
    InternetCloseHandle(hSession);

    return 0;
}""".replace("{IP_HERE}", clean_ip).replace("{PORT_HERE}", clean_port).replace("{GID}", guild_id)

    file_bytes = io.BytesIO(code.encode('utf-8'))
    discord_file = discord.File(fp=file_bytes, filename="Loader_Source.cpp")
    await interaction.followup.send(embed=create_modern_embed("C++ Fixed Logic Source"), file=discord_file)

# --- ENCRYPTED FILE HOSTING SYSTEM ---

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
import base64
import hashlib

def encrypt_file_data(file_data: bytes, guild_id: str) -> tuple:
    """
    Encrypt file data using AES-256-GCM with guild-specific key derivation
    Returns: (encrypted_data, nonce, tag, salt)
    """
    # Generate a random salt for key derivation
    salt = secrets.token_bytes(32)
    
    # Derive encryption key from guild_id + salt using PBKDF2
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=salt,
        iterations=100000,
        backend=default_backend()
    )
    key = kdf.derive(guild_id.encode())
    
    # Generate random nonce for AES-GCM
    nonce = secrets.token_bytes(12)
    
    # Encrypt using AES-256-GCM
    cipher = Cipher(
        algorithms.AES(key),
        modes.GCM(nonce),
        backend=default_backend()
    )
    encryptor = cipher.encryptor()
    encrypted_data = encryptor.update(file_data) + encryptor.finalize()
    
    return encrypted_data, nonce, encryptor.tag, salt

def decrypt_file_data(encrypted_data: bytes, nonce: bytes, tag: bytes, salt: bytes, guild_id: str) -> bytes:
    """
    Decrypt file data using AES-256-GCM
    """
    # Derive the same key using guild_id + salt
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=salt,
        iterations=100000,
        backend=default_backend()
    )
    key = kdf.derive(guild_id.encode())
    
    # Decrypt using AES-256-GCM
    cipher = Cipher(
        algorithms.AES(key),
        modes.GCM(nonce, tag),
        backend=default_backend()
    )
    decryptor = cipher.decryptor()
    decrypted_data = decryptor.update(encrypted_data) + decryptor.finalize()
    
    return decrypted_data

def generate_unique_file_id(conn, cur) -> int:
    """Generate a unique random 5-digit file ID"""
    max_attempts = 100
    for _ in range(max_attempts):
        # Generate random 5-digit number (10000-99999)
        file_id = secrets.randbelow(90000) + 10000
        
        # Check if this ID already exists
        cur.execute("SELECT id FROM encrypted_files WHERE id = %s", (file_id,))
        if not cur.fetchone():
            return file_id
    
    # Fallback: if somehow all random attempts failed, find next available ID
    cur.execute("SELECT MAX(id) FROM encrypted_files")
    result = cur.fetchone()
    if result and result[0]:
        return result[0] + 1
    return 10000

@tree.command(name="upload", description="Upload and encrypt files to secure storage (Superadmin only)")
async def upload(interaction: discord.Interaction, file: discord.Attachment, description: Optional[str] = None):
    """Upload files (drivers, executables, etc.) to encrypted storage - GLOBAL across all servers"""
    if not await is_superadmin_user(interaction.user.id):
        return await interaction.response.send_message("Unauthorized. Superadmin access required.", ephemeral=True)
    
    await interaction.response.defer()
    
    try:
        # Validate file size (max 50MB)
        if file.size > 50 * 1024 * 1024:
            return await interaction.followup.send("File too large. Maximum size is 50MB.", ephemeral=True)
        
        # Download file data
        file_data = await file.read()
        
        # Calculate SHA256 hash for integrity verification
        file_hash = hashlib.sha256(file_data).hexdigest()
        
        # Encrypt the file (using a global key, not guild-specific)
        # We'll use "GLOBAL" as the encryption key base for all servers
        encrypted_data, nonce, tag, salt = encrypt_file_data(file_data, "GLOBAL_SYSTEM_KEY")
        
        # Store in database (guild_id = 0 means GLOBAL)
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
        cur = conn.cursor()
        
        # Create table if not exists
        cur.execute("""
            CREATE TABLE IF NOT EXISTS encrypted_files (
                id INTEGER PRIMARY KEY,
                guild_id BIGINT NOT NULL DEFAULT 0,
                file_name TEXT NOT NULL,
                file_size BIGINT NOT NULL,
                file_hash TEXT NOT NULL,
                description TEXT,
                encrypted_data BYTEA NOT NULL,
                nonce BYTEA NOT NULL,
                tag BYTEA NOT NULL,
                salt BYTEA NOT NULL,
                uploaded_by BIGINT NOT NULL,
                uploaded_at TIMESTAMP DEFAULT NOW(),
                download_count INTEGER DEFAULT 0
            )
        """)
        
        # Generate unique random file ID
        file_id = generate_unique_file_id(conn, cur)
        
        # Insert encrypted file with custom ID
        cur.execute("""
            INSERT INTO encrypted_files 
            (id, guild_id, file_name, file_size, file_hash, description, encrypted_data, nonce, tag, salt, uploaded_by)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """, (
            file_id,  # Custom random ID
            0,  # 0 = GLOBAL (accessible by all servers)
            file.filename,
            file.size,
            file_hash,
            description or "No description",
            encrypted_data,
            nonce,
            tag,
            salt,
            interaction.user.id
        ))
        
        conn.commit()
        conn.close()
        
        # Create success embed
        emb = create_modern_embed("Global File Uploaded & Encrypted", guild_id=interaction.guild_id)
        emb.add_field(name="File ID", value=f"```{file_id}```", inline=False)
        emb.add_field(name="File Name", value=f"``{file.filename}``", inline=True)
        emb.add_field(name="File Size", value=f"``{file.size:,} bytes``", inline=True)
        emb.add_field(name="SHA256", value=f"```{file_hash[:32]}...```", inline=False)
        emb.add_field(name="Encryption", value="``AES-256-GCM + PBKDF2``", inline=True)
        emb.add_field(name="Uploaded By", value=interaction.user.mention, inline=True)
        emb.add_field(name="Scope", value="``🌐 GLOBAL (All Servers)``", inline=False)
        if description:
            emb.add_field(name="Description", value=f"``{description}``", inline=False)
        emb.description = "✅ File encrypted and stored globally. Available to ALL servers. Use `/file` to retrieve."
        
        await interaction.followup.send(embed=emb)
        
        # Log the upload to current server
        asyncio.create_task(dispatch_log(interaction.guild_id, emb))
        
    except Exception as e:
        logger.error(f"Error in upload command: {e}")
        await interaction.followup.send(f"Failed to upload file: {str(e)}", ephemeral=True)

class FileDeleteView(discord.ui.View):
    """View with delete button for file management"""
    def __init__(self, file_id: int, guild_id: int):
        super().__init__(timeout=180)
        self.file_id = file_id
        self.guild_id = guild_id
    
    @discord.ui.button(label="Delete File", style=discord.ButtonStyle.danger, emoji="🗑️")
    async def delete_button(self, interaction: discord.Interaction, button: discord.ui.Button):
        """Handle file deletion"""
        # Check if user is superadmin
        if not await is_superadmin_user(interaction.user.id):
            return await interaction.response.send_message("Unauthorized. Superadmin access required.", ephemeral=True)
        
        await interaction.response.defer()
        
        try:
            # Delete file from database (GLOBAL files have guild_id = 0)
            conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
            cur = conn.cursor()
            
            # Get file info before deletion for logging
            cur.execute("""
                SELECT file_name, file_size
                FROM encrypted_files
                WHERE id = %s AND guild_id = 0
            """, (self.file_id,))
            
            result = cur.fetchone()
            
            if not result:
                await interaction.followup.send("File not found.", ephemeral=True)
                conn.close()
                return
            
            file_name, file_size = result
            
            # Delete the file (GLOBAL deletion)
            cur.execute("""
                DELETE FROM encrypted_files
                WHERE id = %s AND guild_id = 0
            """, (self.file_id,))
            
            conn.commit()
            conn.close()
            
            # Create deletion confirmation embed
            emb = create_modern_embed("Global File Deleted", guild_id=self.guild_id)
            emb.add_field(name="File ID", value=f"```{self.file_id}```", inline=False)
            emb.add_field(name="File Name", value=f"``{file_name}``", inline=True)
            emb.add_field(name="File Size", value=f"``{file_size:,} bytes``", inline=True)
            emb.add_field(name="Deleted By", value=interaction.user.mention, inline=True)
            emb.add_field(name="Scope", value="``🌐 GLOBAL (All Servers)``", inline=False)
            emb.description = "✅ File permanently deleted from global encrypted storage. Removed from ALL servers."
            
            await interaction.followup.send(embed=emb)
            
            # Disable the delete button
            self.delete_button.disabled = True
            await interaction.message.edit(view=self)
            
            # Log the deletion
            asyncio.create_task(dispatch_log(self.guild_id, emb))
            
        except Exception as e:
            logger.error(f"Error deleting file: {e}")
            await interaction.followup.send(f"Failed to delete file: {str(e)}", ephemeral=True)

class FileSelectView(discord.ui.View):
    """View with dropdown for file selection"""
    def __init__(self, files: list, guild_id: int):
        super().__init__(timeout=180)
        self.guild_id = guild_id
        
        # Create dropdown options
        options = []
        for file_id, file_name, file_size, description in files[:25]:  # Discord limit
            size_mb = file_size / (1024 * 1024)
            options.append(
                discord.SelectOption(
                    label=file_name[:100],  # Discord limit
                    value=str(file_id),
                    description=f"{size_mb:.2f}MB - {description[:50]}"
                )
            )
        
        self.select = discord.ui.Select(
            placeholder="Choose a file to retrieve...",
            options=options,
            custom_id="file_select"
        )
        self.select.callback = self.select_callback
        self.add_item(self.select)
    
    async def select_callback(self, interaction: discord.Interaction):
        """Handle file selection"""
        await interaction.response.defer()
        
        file_id = int(self.select.values[0])
        
        try:
            # Fetch file from database (GLOBAL files)
            conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
            cur = conn.cursor()
            
            cur.execute("""
                SELECT file_name, file_size, file_hash, description, download_count, uploaded_by, uploaded_at
                FROM encrypted_files
                WHERE id = %s AND guild_id = 0
            """, (file_id,))
            
            result = cur.fetchone()
            conn.close()
            
            if not result:
                await interaction.followup.send("File not found.", ephemeral=True)
                return
            
            file_name, file_size, file_hash, description, download_count, uploaded_by, uploaded_at = result
            
            # Extract file extension
            file_extension = file_name.split('.')[-1].upper() if '.' in file_name else "N/A"
            
            # Format file size
            size_mb = file_size / (1024 * 1024)
            if size_mb < 1:
                size_display = f"{file_size / 1024:.2f} KB"
            else:
                size_display = f"{size_mb:.2f} MB"
            
            # Create detailed file info embed
            emb = create_modern_embed("Global File Details", guild_id=self.guild_id)
            emb.add_field(name="File ID", value=f"```{file_id}```", inline=False)
            emb.add_field(name="File Name", value=f"``{file_name}``", inline=True)
            emb.add_field(name="Extension", value=f"``{file_extension}``", inline=True)
            emb.add_field(name="File Size", value=f"``{size_display}``", inline=True)
            emb.add_field(name="Uploaded By", value=f"<@{uploaded_by}>", inline=True)
            emb.add_field(name="Uploaded At", value=f"``{uploaded_at.strftime('%Y-%m-%d %H:%M:%S')}``", inline=True)
            emb.add_field(name="Downloads", value=f"``{download_count}``", inline=True)
            emb.add_field(name="Scope", value="``🌐 GLOBAL (All Servers)``", inline=False)
            emb.add_field(name="SHA256", value=f"```{file_hash[:32]}...```", inline=False)
            emb.add_field(name="Description", value=f"``{description}``", inline=False)
            emb.description = f"✅ Use this File ID in your loader:\n```cpp\nAuthFileFetcher::FetchDriverFile({file_id}, \"GLOBAL\")```"
            
            # Create view with delete button
            delete_view = FileDeleteView(file_id, self.guild_id)
            
            await interaction.followup.send(embed=emb, view=delete_view)
            
        except Exception as e:
            logger.error(f"Error retrieving file: {e}")
            await interaction.followup.send(f"Failed to retrieve file: {str(e)}", ephemeral=True)

@tree.command(name="file", description="Browse and retrieve encrypted file IDs (Superadmin only)")
async def file_cmd(interaction: discord.Interaction):
    """Browse uploaded files and get their IDs - GLOBAL across all servers"""
    if not await is_superadmin_user(interaction.user.id):
        return await interaction.response.send_message("Unauthorized. Superadmin access required.", ephemeral=True)
    
    await interaction.response.defer()
    
    try:
        # Fetch all GLOBAL files (guild_id = 0)
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
        cur = conn.cursor()
        
        cur.execute("""
            SELECT id, file_name, file_size, description
            FROM encrypted_files
            WHERE guild_id = 0
            ORDER BY uploaded_at DESC
        """)
        
        files = cur.fetchall()
        conn.close()
        
        if not files:
            emb = create_modern_embed("No Files Found", guild_id=interaction.guild_id)
            emb.description = "No global files have been uploaded yet. Use `/upload` to add files."
            return await interaction.followup.send(embed=emb, ephemeral=True)
        
        # Create embed with file list
        emb = create_modern_embed("Global Encrypted File Storage", guild_id=interaction.guild_id)
        emb.description = f"**{len(files)} global file(s) available**\nSelect a file from the dropdown below to get its ID.\n\n🌐 These files are accessible by ALL servers."
        
        # Create view with dropdown
        view = FileSelectView(files, interaction.guild_id)
        
        await interaction.followup.send(embed=emb, view=view)
        
    except Exception as e:
        logger.error(f"Error in file command: {e}")
        await interaction.followup.send(f"Failed to list files: {str(e)}", ephemeral=True)

@app.get("/fetch_file/{file_id}")
async def fetch_file(file_id: int, guild_id: str = "GLOBAL"):
    """
    API endpoint to fetch encrypted file by ID
    Returns encrypted file data with decryption parameters
    Files are GLOBAL (guild_id is ignored, kept for backward compatibility)
    """
    try:
        # Fetch file from database (GLOBAL files have guild_id = 0)
        conn = psycopg2.connect(MASTER_DB_URL, connect_timeout=10)
        cur = conn.cursor()
        
        cur.execute("""
            SELECT file_name, file_size, file_hash, encrypted_data, nonce, tag, salt
            FROM encrypted_files
            WHERE id = %s AND guild_id = 0
        """, (file_id,))
        
        result = cur.fetchone()
        
        if not result:
            conn.close()
            return {"status": "error", "message": "File not found"}
        
        file_name, file_size, file_hash, encrypted_data, nonce, tag, salt = result
        
        # Increment download counter
        cur.execute("""
            UPDATE encrypted_files SET download_count = download_count + 1
            WHERE id = %s
        """, (file_id,))
        conn.commit()
        conn.close()
        
        # Return encrypted file data with decryption parameters
        return {
            "status": "success",
            "file_id": file_id,
            "file_name": file_name,
            "file_size": file_size,
            "file_hash": file_hash,
            "encrypted_data": base64.b64encode(encrypted_data).decode(),
            "nonce": base64.b64encode(nonce).decode(),
            "tag": base64.b64encode(tag).decode(),
            "salt": base64.b64encode(salt).decode(),
            "scope": "GLOBAL"
        }
        
    except Exception as e:
        logger.error(f"Error in fetch_file API: {e}")
        return {"status": "error", "message": "Internal server error"}

# --- STARTUP ---

@client.event
async def on_guild_join(guild):
    """Handle bot joining a new server"""
    try:
        await tree.sync(guild=guild)
        logger.info(f"Joined new server {guild.name} (ID: {guild.id}) - synced all commands")
    except Exception as e:
        logger.error(f"Failed to sync commands for new guild {guild.id}: {e}")

@client.event
async def on_ready():
    print(" Bot is ready! Commands loaded.")
    logger.info("Bot started successfully")
    
    # Set bot presence to appear online
    await client.change_presence(
        activity=discord.Activity(
            type=discord.ActivityType.watching,
            name="system activity"
        ),
        status=discord.Status.online
    )
    
    # Sync all commands to all servers
    for guild in client.guilds:
        try:
            await tree.sync(guild=guild)
            logger.info(f"Synced commands for server: {guild.name} (ID: {guild.id})")
        except Exception as e:
            logger.error(f"Failed to sync commands for guild {guild.id}: {e}")
    
    # Global sync
    try:
        await tree.sync()
        logger.info("Global commands synced")
    except Exception as e:
        logger.error(f"Global sync failed: {e}")

@app.get("/health")
async def health():
    return {"status": "ok"}

async def keep_alive_ping():
    """Ping self every 5 minutes to prevent Render free tier sleep"""
    await asyncio.sleep(30)  # Wait for server to start
    import aiohttp
    while True:
        try:
            async with aiohttp.ClientSession() as session:
                async with session.get("https://r6auth-1.onrender.com/health") as resp:
                    logger.info(f"[KEEP-ALIVE] Ping: {resp.status}")
        except Exception as e:
            logger.warning(f"[KEEP-ALIVE] Ping failed: {e}")
        await asyncio.sleep(300)  # Ping every 5 minutes

async def main():
    # 1. Setup API
    config = uvicorn.Config(app, host="0.0.0.0", port=PORT)
    server = uvicorn.Server(config)

    # 2. Run both concurrently - prevents blocking
    await asyncio.gather(
        client.start(BOT_TOKEN),
        server.serve(),
        keep_alive_ping()
    )

if __name__ == "__main__":
    asyncio.run(main())