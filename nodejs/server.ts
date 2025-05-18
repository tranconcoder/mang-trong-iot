import app from "@/app";
import { serverConfig } from "@/configs";
import { createServer } from "http";

const server = createServer(app);

server.listen(serverConfig.PORT, () => {
  console.log(
    `Server is running on http://${serverConfig.HOST}:${serverConfig.PORT}`
  );
});

server.on("error", (error) => {
  console.error("Server error:", error);
});

server.on("close", () => {
  console.log("Server is shutting down");
  process.exit(1);
});

process.on("SIGINT", () => server.close());
