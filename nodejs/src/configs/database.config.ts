import dotenv from "dotenv";
import path from "path";

// Load environment variables from .env.development or .env
// Adjust the path if your .env file is located elsewhere or named differently
dotenv.config({ path: path.resolve(__dirname, "../../.env.development") });

const MONGODB_URI = process.env.MONGODB_URI;

if (!MONGODB_URI) {
  console.error(
    "FATAL ERROR: MONGODB_URI is not defined in environment variables."
  );
  process.exit(1); // Exit if URI is not found
}

const databaseConfig = {
  mongodbUri: MONGODB_URI,
  options: {
    // useNewUrlParser: true, // No longer needed in Mongoose 6+
    // useUnifiedTopology: true, // No longer needed in Mongoose 6+
    // useCreateIndex: true, // Not supported
    // useFindAndModify: false, // Not supported
    serverSelectionTimeoutMS: 5000, // Timeout after 5s instead of 30s
    socketTimeoutMS: 45000, // Close sockets after 45 seconds of inactivity
    family: 4, // Use IPv4, skip trying IPv6
  },
};

export default databaseConfig;
