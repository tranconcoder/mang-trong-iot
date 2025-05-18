import { Schema, model } from "mongoose";
import bcrypt from "bcrypt";

export const USER_MODEL_NAME = "User";

export const USER_COLLECTION_NAME = "users";

const userSchema = new Schema(
  {
    email: { type: String, required: true, unique: true },
    password: { type: String, required: true },
    name: { type: String, required: true },
  },
  {
    timestamps: true,
    collection: USER_COLLECTION_NAME,
  }
);

const User = model(USER_MODEL_NAME, userSchema);

export default User;
