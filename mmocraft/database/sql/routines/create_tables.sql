USE $(DatabaseName);

CREATE TABLE player (
    id INT IDENTITY PRIMARY KEY,
    username VARCHAR(64) NOT NULL,
    password BINARY(64) NOT NULL,
    is_admin BIT DEFAULT 0,
    created_at DATETIME2(0) DEFAULT (SYSDATETIME())
);

CREATE TABLE player_game_data (
    player_id INT PRIMARY KEY,
    CONSTRAINT FK_PlayerID FOREIGN KEY (player_id) REFERENCES player (id)
    ON DELETE CASCADE
    ON UPDATE CASCADE,
    latest_position BIGINT DEFAULT 0,
    spawn_position BIGINT DEFAULT 0,
    level TINYINT DEFAULT 1,
);

INSERT INTO player (username, password, is_admin) VALUES (
    '$(AdminPlayerName)',
    dbo.GetPasswordHash('$(AdminPlayerPassword)'),
    1
);

INSERT INTO player_game_data (player_id) VALUES (1);