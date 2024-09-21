use mmocraft

db.createCollection("player_session")

db.player_session.createIndex(
    { username: 1 },
    { name: "Username-Search-Index" }
)

db.player_session.createIndex(
    { expired_at: 1 },
    { 
        name: "Session-TTL-Index",
        expireAfterSeconds: 0
    }	
)