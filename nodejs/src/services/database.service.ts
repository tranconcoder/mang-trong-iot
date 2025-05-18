import mongoose, { type ConnectOptions } from "mongoose";
import databaseConfig from "@/configs/database.config";

const connectDB = async () => {
  try {
    console.log("Attempting to connect to MongoDB...");
    await mongoose.connect(
      databaseConfig.mongodbUri,
      databaseConfig.options as ConnectOptions
    );
    console.log("MongoDB connected successfully!");
  } catch (err: any) {
    console.error("MongoDB connection error:", err.message);
    // Exit process with failure
    process.exit(1);
  }
};

mongoose.connection.on("disconnected", () => {
  console.log("MongoDB disconnected.");
});

mongoose.connection.on("reconnected", () => {
  console.log("MongoDB reconnected.");
});

// Graceful shutdown
process.on("SIGINT", async () => {
  await mongoose.connection.close();
  console.log("MongoDB connection closed due to app termination.");
  process.exit(0);
});

export { connectDB };
