import dotenv from "dotenv";
import path from "path";

// Ensure environment variables are loaded
// This might be redundant if your app.ts or another config already loads them comprehensively
dotenv.config({ path: path.resolve(__dirname, "../../.env.development") });

const JWT_SECRET = process.env.JWT_SECRET;

if (!JWT_SECRET) {
  console.error(
    "FATAL ERROR: JWT_SECRET is not defined in environment variables."
  );
  console.log("Please add JWT_SECRET to your .env.development file.");
  process.exit(1);
}

export default {
  SECRET: JWT_SECRET,
  EXPIRES_IN: process.env.JWT_EXPIRES_IN || "1h", // Default token expiration to 1 hour
};
