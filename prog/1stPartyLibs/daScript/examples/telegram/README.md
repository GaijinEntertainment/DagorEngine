# daspkg-telegram example

Telegram echo bot using the `dasTelegram` package installed via daspkg.

## Setup

```
daspkg install
```

This reads `.das_package` and installs `dasTelegram` into `modules/`.

Or install directly:

```
daspkg install dasTelegram
```

## Run

Set your bot token (from [@BotFather](https://t.me/BotFather)):

```
export TELEGRAM_BOT_TOKEN="your-token-here"
```

Or create a `.bot_token` file in this directory.

Then run:

```
daslang echo_bot.das --
```

The bot echoes back any text message you send it.
