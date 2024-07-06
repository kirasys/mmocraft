USE $(DatabaseName);

CREATE TABLE player (
    id INT IDENTITY PRIMARY KEY,
    username VARCHAR(64) NOT NULL,
    password BINARY(64) NOT NULL,
    
    latest_position BIGINT DEFAULT 0,
    spawn_position BIGINT DEFAULT 0,
    level TINYINT DEFAULT 1,

    created_at DATETIME2(0) DEFAULT (SYSDATETIME())
);

INSERT INTO player (username, password) VALUES (
    '$(AdminPlayerName)',
    dbo.GetPasswordHash('$(AdminPlayerPassword)')
);