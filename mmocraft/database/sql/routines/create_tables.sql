USE $(DatabaseName);

CREATE TABLE player (
    id INT IDENTITY,
    username VARCHAR(64) NOT NULL,
    password BINARY(64) NOT NULL,
    is_admin BIT DEFAULT 0,
    created_at DATETIME2(0) DEFAULT (SYSDATETIME())

    CONSTRAINT PK_player_id PRIMARY KEY NONCLUSTERED (id)
);

CREATE CLUSTERED INDEX idx_player_username ON player (username)

CREATE TABLE player_game_data (
    player_id INT PRIMARY KEY,
    CONSTRAINT FK_PlayerID FOREIGN KEY (player_id) REFERENCES player (id)
    ON DELETE CASCADE
    ON UPDATE CASCADE,

    /*
    Gamedata:
        latest_position BIGINT,
        spawn_position BIGINT,
        level INT,
        exp INT
    */
    gamedata BINARY(64)
);

INSERT INTO player (username, password, is_admin) VALUES (
    '$(AdminPlayerName)',
    dbo.GetPasswordHash('$(AdminPlayerPassword)'),
    1
);

INSERT INTO player_game_data (player_id) VALUES (1);

GO