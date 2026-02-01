#!/usr/bin/env python3

import json
from datetime import datetime

def get_data():
    data = {
        "users": [
            {"id": 1, "name": "Alice", "email": "alice@example.com", "status": "active"},
            {"id": 2, "name": "Bob", "email": "bob@example.com", "status": "inactive"},
            {"id": 3, "name": "Charlie", "email": "charlie@example.com", "status": "active"},
            {"id": 4, "name": "Diana", "email": "diana@example.com", "status": "active"},
            {"id": 5, "name": "Eve", "email": "eve@example.com", "status": "inactive"}
        ],
        "stats": {
            "total_users": 5,
            "active_users": 3,
            "inactive_users": 2,
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        }
    }
    return data

def generate_html(data):
    html = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>User Dashboard</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }}
        .container {{ max-width: 1200px; margin: 0 auto; }}
        h1 {{ color: #333; border-bottom: 3px solid #007bff; padding-bottom: 10px; }}
        .stats {{ display: grid; grid-template-columns: repeat(4, 1fr); gap: 20px; margin-bottom: 30px; }}
        .stat-card {{ background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); text-align: center; }}
        .stat-card h3 {{ margin: 0 0 10px 0; color: #666; font-size: 14px; }}
        .stat-card .value {{ font-size: 32px; font-weight: bold; color: #007bff; }}
        table {{ width: 100%; border-collapse: collapse; background: white; border-radius: 8px; overflow: hidden; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        th {{ background-color: #007bff; color: white; padding: 12px; text-align: left; }}
        td {{ padding: 12px; border-bottom: 1px solid #ddd; }}
        tr:hover {{ background-color: #f9f9f9; }}
        .status {{ padding: 4px 8px; border-radius: 4px; font-size: 12px; font-weight: bold; }}
        .status.active {{ background-color: #d4edda; color: #155724; }}
        .status.inactive {{ background-color: #f8d7da; color: #721c24; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>User Dashboard</h1>
        <div class="stats">
            <div class="stat-card">
                <h3>Total Users</h3>
                <div class="value">{total_users}</div>
            </div>
            <div class="stat-card">
                <h3>Active Users</h3>
                <div class="value">{active_users}</div>
            </div>
            <div class="stat-card">
                <h3>Inactive Users</h3>
                <div class="value">{inactive_users}</div>
            </div>
            <div class="stat-card">
                <h3>Last Updated</h3>
                <div class="value" style="font-size: 14px; color: #666;">{timestamp}</div>
            </div>
        </div>
        <table>
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Name</th>
                    <th>Email</th>
                    <th>Status</th>
                </tr>
            </thead>
            <tbody>
                {users_rows}
            </tbody>
        </table>
    </div>
</body>
</html>"""
    
    users_rows = ""
    for user in data["users"]:
        users_rows += f"""                <tr>
                    <td>{user['id']}</td>
                    <td>{user['name']}</td>
                    <td>{user['email']}</td>
                    <td><span class="status {user['status']}">{user['status'].upper()}</span></td>
                </tr>
"""
    
    html = html.format(
        total_users=data["stats"]["total_users"],
        active_users=data["stats"]["active_users"],
        inactive_users=data["stats"]["inactive_users"],
        timestamp=data["stats"]["timestamp"],
        users_rows=users_rows
    )
    
    return html

if __name__ == "__main__":
    data = get_data()
    html_output = generate_html(data)
    print(html_output)
