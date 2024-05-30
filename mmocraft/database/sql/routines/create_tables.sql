USE $(DatabaseName);

CREATE TABLE player (
    id int IDENTITY PRIMARY KEY,
    username VARCHAR(64) NOT NULL,
    password VARBINARY(64) NOT NULL,
    created_at DATETIME2(0) DEFAULT (SYSDATETIME())
);

INSERT INTO player (username, password) VALUES (
    '$(AdminPlayerName)',
    dbo.GetPasswordHash('$(AdminPlayerPassword)')
);